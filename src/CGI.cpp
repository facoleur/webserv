#include "RequestRouter.hpp"
#include "Response.hpp"
#include <sys/wait.h>

int RequestRouter::executeCgi(const ServerConfig& serverConfig, const LocationConfig& locationConfig,
                              const Request& request, const std::string& scriptPath, const std::string& interpreter,
                              std::string& responseBody, std::map<std::string, std::string>& responseHeaders,
                              int& statusCode, std::string& statusMessage) const {

    responseBody.clear();
    responseHeaders.clear();
    statusCode    = 200;
    statusMessage = "OK";

    std::vector<std::string> envStorage;
    std::string protocol      = request.getProtocolVersion().empty() ? "HTTP/1.1" : request.getProtocolVersion();
    std::string host          = request.getHeader(HOST);
    std::string contentType   = request.getHeader(CONTENT_TYPE);
    std::string contentLength = request.getHeader(CONTENT_LENGTH);
    std::string documentRoot  = locationConfig.root.empty() ? serverConfig.root : locationConfig.root;

    envStorage.push_back("REQUEST_METHOD=" + methodToString(request.getMethod()));
    envStorage.push_back("SCRIPT_FILENAME=" + scriptPath);
    envStorage.push_back("QUERY_STRING=" + request.getQueryString());
    envStorage.push_back("SERVER_PROTOCOL=" + protocol);
    envStorage.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envStorage.push_back("SERVER_SOFTWARE=webserv");
    envStorage.push_back("REDIRECT_STATUS=200");
    envStorage.push_back("SCRIPT_NAME=" + request.getPath());
    envStorage.push_back("PATH_INFO=" + request.getPath());
    envStorage.push_back("REQUEST_URI=" + request.getPath());
    envStorage.push_back("DOCUMENT_ROOT=" + documentRoot);
    envStorage.push_back("SERVER_NAME=" + (serverConfig.host.empty() ? std::string("localhost") : serverConfig.host));
    if (!serverConfig.listen_ports.empty())
        envStorage.push_back("SERVER_PORT=" + toString(serverConfig.listen_ports[0]));
    if (!host.empty())
        envStorage.push_back("HTTP_HOST=" + host);
    if (!contentType.empty())
        envStorage.push_back("CONTENT_TYPE=" + contentType);
    if (!contentLength.empty())
        envStorage.push_back("CONTENT_LENGTH=" + contentLength);

    int stdinPipe[2];
    int stdoutPipe[2];
    if (pipe(stdinPipe) == -1)
        return -1;
    if (pipe(stdoutPipe) == -1) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        return -1;
    }

    if (pid == 0) {
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        dup2(stdinPipe[0], STDIN_FILENO);
        dup2(stdoutPipe[1], STDOUT_FILENO);
        close(stdinPipe[0]);
        close(stdoutPipe[1]);

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(interpreter.c_str()));
        argv.push_back(const_cast<char*>(scriptPath.c_str()));
        argv.push_back(NULL);

        std::vector<char*> envp;
        for (size_t i = 0; i < envStorage.size(); ++i) {
            envp.push_back(const_cast<char*>(envStorage[i].c_str()));
        }
        envp.push_back(NULL);

        execve(interpreter.c_str(), &argv[0], &envp[0]);
        _exit(1);
    }

    close(stdinPipe[0]);
    close(stdoutPipe[1]);

    const std::string& body    = request.getBody();
    size_t             written = 0;
    while (written < body.size()) {
        ssize_t chunk = write(stdinPipe[1], body.data() + written, body.size() - written);
        if (chunk <= 0) {
            close(stdinPipe[1]);
            close(stdoutPipe[0]);
            waitpid(pid, NULL, 0);
            return -1;
        }
        written += static_cast<size_t>(chunk);
    }
    close(stdinPipe[1]);

    std::string output;
    char        buffer[4096];
    ssize_t     bytes = 0;
    while ((bytes = read(stdoutPipe[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, static_cast<size_t>(bytes));
    }
    close(stdoutPipe[0]);
    if (bytes == -1) {
        waitpid(pid, NULL, 0);
        return -1;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) == -1)
        return -1;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return -1;

    std::string::size_type headerEnd = output.find("\r\n\r\n");
    size_t                 delimiter = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
        delimiter = 2;
    }

    std::string headersBlock;
    if (headerEnd != std::string::npos) {
        headersBlock = output.substr(0, headerEnd);
        responseBody = output.substr(headerEnd + delimiter);
    } else {
        responseBody = output;
    }

    if (!headersBlock.empty()) {
        std::istringstream iss(headersBlock);
        std::string        line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            if (line.empty())
                continue;
            std::string::size_type sep = line.find(':');
            if (sep == std::string::npos)
                continue;
            std::string headerName  = line.substr(0, sep);
            std::string headerValue = trimString(line.substr(sep + 1));
            std::string lowered     = toLower(headerName);
            if (lowered == "status") {
                std::istringstream statusStream(headerValue);
                int                code = 0;
                statusStream >> code;
                if (statusStream && code >= 100 && code <= 599) {
                    statusCode = code;
                    std::string text;
                    std::getline(statusStream, text);
                    text = trimString(text);
                    if (!text.empty())
                        statusMessage = text;
                    else
                        statusMessage.clear();
                }
                continue;
            }
            responseHeaders[headerName] = headerValue;
        }
    }

    return 0;
}

Response RequestRouter::handleCgi(const Request& req, const std::string& path, const ServerConfig& serverConfig,
                                  const LocationConfig& resolvedConfig) {
    if (!isSubPath(resolvedConfig.root, path))
        return makeErrorResponse(FORBIDDEN);

    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return makeErrorResponse(NOT_FOUND);
    if (!S_ISREG(st.st_mode) || access(path.c_str(), R_OK) != 0)
        return makeErrorResponse(FORBIDDEN);

    std::string interpreter = getCgiInterpreter(path, resolvedConfig);
    if (interpreter.empty())
        return makeErrorResponse(BAD_GATEWAY);

    std::string                        cgiBody;
    std::map<std::string, std::string> cgiHeaders;
    int                                statusCode    = 200;
    std::string                        statusMessage = "OK";
    if (executeCgi(serverConfig, resolvedConfig, req, path, interpreter, cgiBody, cgiHeaders, statusCode,
                   statusMessage) != 0) {
        return makeErrorResponse(BAD_GATEWAY);
    }

    if (cgiHeaders.find("Content-Type") == cgiHeaders.end())
        cgiHeaders["Content-Type"] = "text/html";
    if (cgiHeaders.find("Content-Length") == cgiHeaders.end())
        cgiHeaders["Content-Length"] = toString(cgiBody.size());

    Response response;
    response.setStatusCode(static_cast<enum statusCode>(statusCode));

    for (std::map<std::string, std::string>::const_iterator headerIt = cgiHeaders.begin(); headerIt != cgiHeaders.end();
         ++headerIt) {
        std::string lower = toLower(headerIt->first);
        if (lower == "content-length")
            response.setHeader(CONTENT_LENGTH, headerIt->second);
        else if (lower == "content-type")
            response.setHeader(CONTENT_TYPE, headerIt->second);
        else if (lower == "location")
            response.setHeader(LOCATION, headerIt->second);
        else if (lower == "transfer-encoding")
            response.setHeader(TRANSFER_ENCODING, headerIt->second);
        else if (lower == "connection")
            response.setHeader(CONNECTION, headerIt->second);
        // else
        //     response.addHeader(headerIt->first, headerIt->second);
    }
    response.setBody(cgiBody);
    return response;
}
