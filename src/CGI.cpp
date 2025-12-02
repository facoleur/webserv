// CGI.cpp

#include <sstream>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "CGI.hpp"
#include "Enums.hpp"
#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include "Server.hpp"

void Server::generateResponseFromCgiOutput(std::string output, Response& response) {

    std::string                        responseBody;
    std::map<std::string, std::string> responseHeaders;
    int                                statusCode    = 200;
    std::string                        statusMessage = "OK";

    // preparing the response
    response.setStatusCode(
        static_cast<enum statusCode>(statusCode)); // ALWAYS 200 ? I guess we only use this function when CGI went ok
    response.setBody(responseBody);

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

    // typedef std::map<requestHeaders, std::string> headersMap;
    response.setHeaders(responseHeaders);

    if (responseHeaders.find("Content-Type") == responseHeaders.end())
        responseHeaders["Content-Type"] = "text/html";
    if (responseHeaders.find("Content-Length") == responseHeaders.end())
        responseHeaders["Content-Length"] = toString(responseBody.size());

    for (std::map<std::string, std::string>::const_iterator headerIt = responseHeaders.begin();
         headerIt != responseHeaders.end(); ++headerIt) {
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
}

std::vector<std::string> RequestRouter::storeCgiEnv(const Request& request, const LocationConfig& locationConfig,
                                                    const ServerConfig& serverConfig,
                                                    const std::string&  scriptPath) const {
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

    return envStorage;
}

Response RequestRouter::prepareCgi(Request& req, const std::string& path, const ServerConfig& serverConfig,
                                   const LocationConfig& resolvedConfig) {

    // check path
    if (!isSubPath(resolvedConfig.root, path))
        return makeErrorResponse(FORBIDDEN);

    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return makeErrorResponse(NOT_FOUND);
    if (!S_ISREG(st.st_mode) || access(path.c_str(), R_OK) != 0)
        return makeErrorResponse(FORBIDDEN);

    // set Cgi interpreter
    std::string interpreter = getCgiInterpreter(path, resolvedConfig);
    if (interpreter.empty())
        return makeErrorResponse(BAD_GATEWAY);
    req.cgiInfo.setInterpreter(interpreter);

    // prepare env variables
    std::vector<std::string> envStorage = storeCgiEnv(req, resolvedConfig, serverConfig, path);
    req.cgiInfo.setEnvStorage(envStorage);

    Response response;
    response.setMustLaunchCgi(true);
    return response;
}

int Server::setupCgiPipes(int (&stdinPipe)[2], int (&stdoutPipe)[2]) {
    if (pipe(stdinPipe) == -1)
        return -1;

    int flags = fcntl(stdinPipe[1], F_GETFL);
    if (flags == -1 || fcntl(stdinPipe[1], F_SETFL, flags | O_NONBLOCK) == -1) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        return -1;
    }

    if (pipe(stdoutPipe) == -1) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        return -1;
    }

    flags = fcntl(stdoutPipe[0], F_GETFL);
    if (flags == -1 || fcntl(stdoutPipe[0], F_SETFL, flags | O_NONBLOCK) == -1) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        return -1;
    }
    return 0;
}

int Server::writeToCgi(int (&stdinPipe)[2], int (&stdoutPipe)[2], Request& request) {
    pid_t pid = request.cgiInfo.getCgiPID();

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
        request.cgiInfo.setLastActive(time(NULL));
    }
    close(stdinPipe[1]);
    return 0;
}

int Server::readFromCgi(int (&stdoutPipe)[2], Request& request) {
    pid_t       pid = request.cgiInfo.getCgiPID();
    std::string output;
    char        buffer[4096];
    ssize_t     bytes = 0;
    while ((bytes = read(stdoutPipe[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, static_cast<size_t>(bytes));
        request.cgiInfo.setLastActive(time(NULL));
    }
    close(stdoutPipe[0]);
    if (bytes == -1) {
        waitpid(pid, NULL, 0);
        return -1;
    }
}

int Server::waitForCgiTermination(pid_t pid, Request& req) {
    (void)req;
    int status = 0;
    int ret    = waitpid(pid, &status, WNOHANG);

    if (ret == -1) // If an error is detected or a caught signal aborts the call, a value of -1 is returned and
                   // errno is set to indicate the error.
                   // tear down the CGI state
        return -1; // handle ECHILD ? i.e. no existing CGI ? "The calling process has no existing unwaited-for child
    // processes."
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return -1;
    // if (ret == 0)    // child still running, leave the pipes registered and keep enforcing timeouts
    //                  // do nothing, continue server ?
    //     if (ret > 0) // child exited => clean up pipes, parse buffered output, transition the request to DONE (or
    //     error
    //                  // if !WIFEXITED/WEXITSTATUS != 0)
}

int Server::launchCgi(Request& request) {

    // get prepared CGI info
    const std::string              scriptPath  = request.cgiInfo.getScriptPath();
    const std::string              interpreter = request.cgiInfo.getInterpreter();
    const std::vector<std::string> envStorage  = request.cgiInfo.getEnvStorage();

    // setup CGI pipes
    int stdinPipe[2];
    int stdoutPipe[2];
    if (setupCgiPipes(stdinPipe, stdoutPipe) == -1)
        return -1;
    request.cgiInfo.setWriteFd(stdinPipe[1]);
    request.cgiInfo.setReadFd(stdoutPipe[0]);

    // fork
    pid_t pid = fork();
    if (pid == -1) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        return -1;
    }
    request.cgiInfo.exists = true; // CGI was started

    if (pid == 0) { // child process
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
    } else
        request.cgiInfo.setCgiPID(pid);

    // write body to CGI
    if (writeToCgi(stdinPipe, stdoutPipe, request) == -1)
        return -1;

    // read output from CGI
    if (readFromCgi(stdoutPipe, request) == -1)
        return -1;

    // wait for CGI termination
    if (waitForCgiTermination(pid, request) == -1)
        return -1;

    return 0;
}
