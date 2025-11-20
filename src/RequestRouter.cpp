// RequestRouter.cpp

#include "RequestRouter.hpp"
#include <cctype>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

RequestRouter::RequestRouter() {
}
RequestRouter::~RequestRouter() {
}

bool RequestRouter::resourceExists(const std::string& path, const Request& req) {
    (void)req;
    struct stat info;
    return (stat(path.c_str(), &info) == 0);
}

bool RequestRouter::isMethodAllowed(const Request& req, const LocationConfig& config) {
    for (std::set<enum requestMethod>::const_iterator it = config.methods.begin(); it != config.methods.end(); ++it) {
    }

    return (config.methods.find(req.getMethod()) != config.methods.end());
}

bool RequestRouter::isCgiRequest(const std::string& path, const LocationConfig& config) {
    return !getCgiInterpreter(path, config).empty();
}

std::string RequestRouter::getCgiInterpreter(const std::string& path, const LocationConfig& config) const {
    const std::map<std::string, std::string>& cgi_ext = config.cgi_map;
    if (cgi_ext.empty())
        return "";

    std::string::size_type dot = path.find_last_of('.');
    if (dot != std::string::npos) {
        std::string                                        ext = path.substr(dot);
        std::map<std::string, std::string>::const_iterator it  = cgi_ext.find(ext);
        if (it != cgi_ext.end())
            return it->second;
        if (dot + 1 < path.size()) {
            std::string alt = path.substr(dot + 1);
            it              = cgi_ext.find(alt);
            if (it != cgi_ext.end())
                return it->second;
            if (!alt.empty()) {
                std::string withDot = "." + alt;
                it                  = cgi_ext.find(withDot);
                if (it != cgi_ext.end())
                    return it->second;
            }
        }
    }
    return "";
}

void RequestRouter::resolveAbsolutePath(std::string& path) {
    std::string::size_type pos = path.find("http://");
    path.erase(pos, 7);

    pos = path.find("/");
    path.erase(0, pos);
}

std::string RequestRouter::resolvePath(const Request& req, const std::string& root, const std::string& location) {
    (void)req;
    std::string path = req.getPath();
    // std::cout << "path: " << path << std::endl;
    // std::cout << "location: " << location << std::endl;

    path = "/" + path.substr(location.size());

    // if (startsWith(path, "http://"))
    //     resolveAbsolutePath(path);
    // else if (!startsWith(path, "/")) {
    //     throw std::runtime_error("Relative path doesn't start with /");
    // }

    if (path.empty())
        path = "/";

    if (path.find("%") != std::string::npos || path.find("=") != std::string::npos ||
        path.find("../") != std::string::npos || path.find("./") != std::string::npos ||
        path.find("=") != std::string::npos || path.find("&") != std::string::npos)
        throw std::runtime_error("Not accepted in path");

    replace(path, "//", "/");

    std::string fullPath = root;

    fullPath.append(path);

    while (fullPath.find("//") != std::string::npos) {
        replace(fullPath, "//", "/");
    }

    if (fullPath[0] == '/')
        fullPath.erase(fullPath.begin());

    return fullPath;
}

std::string RequestRouter::getMimeType(const std::string& path) {
    std::map<std::string, std::string> mime;
    mime["html"] = "text/html";
    mime["htm"]  = "text/html";
    mime["css"]  = "text/css";
    mime["js"]   = "application/javascript";
    mime["jpg"]  = "image/jpeg";
    mime["jpeg"] = "image/jpeg";
    mime["png"]  = "image/png";
    mime["gif"]  = "image/gif";

    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos)
        return "text/plain";

    std::string extension = tolower(path.substr(++pos));
    std::string mimetype  = mime[extension];

    if (mimetype == "")
        return "text/plain";

    return mime[extension];
}

Response RequestRouter::handleGet(const Request& req, std::string& path, const LocationConfig& config) {
    (void)req;
    Response res;

    if (isDirectory(path)) {

        if (!path.empty() && path[path.size() - 1] != '/') {
            res.setStatusCode(REDIRECT);
            res.setHeader(LOCATION, path + "/");
            return res;
        }

        for (size_t i = 0; i < config.index_files.size(); ++i) {
            std::string indexPath = path + config.index_files[i];
            if (!resourceExists(indexPath, req))
                continue;

            std::ifstream file(indexPath.c_str());
            if (!file.is_open())
                continue;

            std::string body = readFile(file);
            res.setHeader(CONTENT_TYPE, getMimeType(indexPath));
            res.setHeader(CONTENT_LENGTH, toString(body.size()));
            res.setBody(body);
            return res;
        }

        if (config.autoindex) {
            return makeAutoindexResponse(path, config.path);
        } else {
            // std::cout << "forbidden here!" << std::endl;
            return makeErrorResponse(FORBIDDEN);
        }
    }

    if (!resourceExists(path, req))
        return makeErrorResponse(NOT_FOUND);

    std::ifstream file(path.c_str());
    if (!file.is_open())
        return makeErrorResponse(NOT_FOUND);

    res.setBody(readFile(file));
    res.setHeader(CONTENT_TYPE, getMimeType(path));
    return res;
}

std::string generateUploadName() {
    std::ostringstream oss;
    oss << "upload_" << std::time(0);
    return oss.str();
}

Response RequestRouter::handlePost(const Request& req, const std::string& path, const LocationConfig& config) {
    // std::cout << "body: " << req.getBody() << std::endl;
    // std::cout << "body.size(): " << req.getBody().size() << std::endl;
    // std::cout << "config.path: " << config.path << std::endl;

    if (toSizet(req.getHeader(CONTENT_LENGTH)) > config.client_max_body_size) {
        return makeErrorResponse(CONTENT_TOO_LARGE);
    }

    if (config.upload_enable == false) {

        // std::cout << "ici" << std::endl;
        return makeErrorResponse(FORBIDDEN);
    }

    std::string basename = path;
    size_t      pos      = path.find_last_of('/');
    if (pos != std::string::npos)
        basename = path.substr(pos + 1);

    std::string uploadPath;
    std::string filename = generateUploadName();
    if (!config.upload_store.empty())
        uploadPath = config.root + config.upload_store + "/" + filename;

    while (uploadPath.find("//") != std::string::npos) {
        replace(uploadPath, "//", "/");
    }

    // std::cout << "uploadPath: " << uploadPath << std::endl;
    std::ofstream out(uploadPath.c_str(), std::ios::binary);
    if (!out.is_open()) {
        // std::cout << "500 ici" << std::endl;
        return makeErrorResponse(INTERNAL_SERVER_ERROR);
    }

    out.write(req.getBody().c_str(), req.getBody().size());

    // std::cout << "uploadPath: " << uploadPath << std::endl;

    Response res;
    res.setHeader(LOCATION, uploadPath);
    res.setStatusCode(CREATED);
    return res;
}

Response RequestRouter::handleDelete(const Request& req, const std::string& path, const LocationConfig& config) {
    // if dir -> 403 (design choice, otherwise we must delete recursively dir entries)
    (void)req;
    (void)config;

    if (isDirectory(path)) {
        return makeErrorResponse(FORBIDDEN);
    }

    if (access(path.c_str(), W_OK)) {
        return makeErrorResponse(FORBIDDEN);
    }

    if (std::remove(path.c_str()) != 0) {
        return makeResponse(INTERNAL_SERVER_ERROR);
    }

    return makeResponse(NO_CONTENT);
}

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

Response RequestRouter::makeResponse(enum statusCode statusCode) {
    Response res;
    res.setStatusCode(statusCode);

    return res;
}

std::string RequestRouter::generateErrorHtml(enum statusCode status) {
    std::ifstream templateHtml("www/templates/error.html");
    std::string   html = readFile(templateHtml);

    replaceVariables(html, "statusCode", toString(status));
    replaceVariables(html, "message", toString(status));

    return html;
}

Response RequestRouter::makeErrorResponse(enum statusCode statusCode) {
    Response res;
    res.setBody(generateErrorHtml(statusCode));
    res.setStatusCode(statusCode);

    return res;
}

Response RequestRouter::makeAutoindexResponse(const std::string& path, const std::string& location) {
    DIR* dirstream = opendir(path.c_str());

    std::string loc = location;
    if (loc[loc.size() - 1] != '/')
        loc.push_back('/');

    std::vector<AutoIndexItem> files;
    files.push_back(AutoIndexItem(getParentDir(path), "../", 0, T_DIR, ""));

    while (dirent* f = readdir(dirstream)) {

        if (std::string(f->d_name) == "." || std::string(f->d_name) == "..")
            continue;

        std::string name(f->d_name);
        std::string fileloc = location + "/" + f->d_name;

        std::string fullpath = path + "/" + f->d_name;
        struct stat st;
        stat(fullpath.c_str(), &st);

        time_t     lastModified = st.st_mtime;
        char       lastModifedReadable[64];
        struct tm* tm = localtime(&lastModified);
        strftime(lastModifedReadable, sizeof(lastModifedReadable), "%Y-%m-%d %H:%M:%S", tm);

        enum autoIndexType type = T_FILE;
        if (f->d_type == DT_DIR) {
            type = T_DIR;
            name.push_back('/');
        }
        files.push_back(AutoIndexItem(fileloc, name, st.st_size, type, lastModifedReadable));
    }

    std::string html = AutoIndex::fillTemplate(loc, files);

    Response res;
    res.setStatusCode(OK);
    res.setHeader(CONTENT_LENGTH, toString(html.size()));
    res.setBody(html);

    return res;
}

const LocationConfig* findLocationConfig(const std::string& path, const ServerConfig& config) {
    const LocationConfig* best    = NULL;
    size_t                bestLen = 0;
    for (std::vector<LocationConfig>::const_iterator it = config.locations.begin(); it != config.locations.end();
         ++it) {
        if (path.compare(0, it->path.size(), it->path) == 0 && it->path.size() > bestLen) {

            best    = &(*it);
            bestLen = it->path.size();
        }
    }
    return best;
}

const LocationConfig resolveConfig(const ServerConfig& server, const LocationConfig* location,
                                   std::string& locationPath) {

    LocationConfig resolved;

    if (location) {
        locationPath = location->path;
        resolved     = *location;
    }

    if (resolved.methods.empty()) {
        resolved.methods = server.methods;
    }
    if (resolved.root.empty()) {
        resolved.root = server.root;
    }
    if (resolved.index_files.empty()) {
        resolved.index_files = server.index_files;
    }
    if (resolved.cgi_map.empty()) {
        resolved.cgi_map = server.cgi_map;
    }

    return resolved;
}

Response RequestRouter::makeRedirectResponse(const std::string& location) {
    Response res;
    res.setStatusCode(REDIRECT);
    res.setHeader(LOCATION, location);
    res.setBody("");
    return res;
}

Response RequestRouter::route(const Request& req, const ServerConfig& config) {

    if (req.getStatusCode() != NO_STATUS) {
        makeErrorResponse(req.getStatusCode()); // can be 413 CONTENT_TOO_LARGE, for example
        DEBUG_LOG("RequestRouter.route(): status already set before to: " + ReasonPhrase::get(req_.getStatusCode()));
    }

    Request req_;

    req_.setMethod("GET");
    req_.setPath("/delete/asd");
    req_.setProtocolVersion("HTTP/1.1");
    // req.setHeader(CONTENT_LENGTH, toString(10));
    // req.setBody("helloWorld");

    std::cout << req_ << std::endl;

    Response response;

    // static/HTTP-based semantic checks
    if (req_.validateRequest(response) == INVALID_REQUEST) {
        return makeErrorResponse(response.getStatusCode());
    }

    // dynamic/config-based checks
    const LocationConfig* locationConfig = findLocationConfig(req_.getPath(), config);

    std::string           locationPath;
    const LocationConfig& resolvedConfig = resolveConfig(config, locationConfig, locationPath);

    if (resolvedConfig.redirect.status) {
        return makeRedirectResponse(resolvedConfig.redirect.target);
    }

    std::string resolvedPath;

    try {
        resolvedPath = resolvePath(req_, resolvedConfig.root, locationPath);
    } catch (std::exception&) {
        return makeErrorResponse(BAD_REQUEST);
    }

    if (!resourceExists(resolvedPath, req_)) {
        return makeErrorResponse(NOT_FOUND);
    }

    if (!isMethodAllowed(req_, resolvedConfig)) {
        return makeErrorResponse(NOT_ALLOWED);
    }

    if (isCgiRequest(resolvedPath, resolvedConfig))
        return handleCgi(req, resolvedPath, config, resolvedConfig);

    switch (req_.getMethod()) {
        case GET:
            return handleGet(req_, resolvedPath, resolvedConfig);
        case POST:
            return handlePost(req_, resolvedPath, resolvedConfig);
        case DELETE:
            return handleDelete(req_, resolvedPath, resolvedConfig);
        default:
            return makeErrorResponse(BAD_REQUEST);
    }
}
