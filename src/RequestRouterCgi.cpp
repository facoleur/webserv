// RequestRouterCgi.cpp

#include <sstream>
#include <unistd.h>

#include "Enums.hpp"
#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

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

    // check path to script
    if (!isSubPath(resolvedConfig.root, path))
        return makeErrorResponse(FORBIDDEN);

    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return makeErrorResponse(NOT_FOUND);
    if (!S_ISREG(st.st_mode) || access(path.c_str(), R_OK) != 0)
        return makeErrorResponse(FORBIDDEN);
    req.cgiInfo.setScriptPath(path);

    // set CGI interpreter
    std::string interpreter = getCgiInterpreter(path, resolvedConfig);
    if (interpreter.empty())
        return makeErrorResponse(BAD_GATEWAY);
    req.cgiInfo.setInterpreter(interpreter);

    // prepare CGI env variables
    std::vector<std::string> envStorage = storeCgiEnv(req, resolvedConfig, serverConfig, path);
    req.cgiInfo.setEnvStorage(envStorage);

    // set CGI owning request
    req.cgiInfo.setRequest(req);

    Response response;
    response.setMustLaunchCgi(true);
    return response;
}

Response RequestRouter::generateResponseFromCgiOutput(Request& req, Response& response, std::string output) {

    std::string                        responseBody;
    std::map<std::string, std::string> responseHeaders;
    int                                statusCode    = 200;
    std::string                        statusMessage = "OK";

    // check if there was an error
    if (req.getStatusCode() != NO_STATUS) {
        std::string reasonPhrase(ReasonPhrase::get(req.getStatusCode()));
        DEBUG_LOG("RequestRouter::generateResponseFromCgiOutput(): status already set before to: " + reasonPhrase);
        return makeErrorResponse(req.getStatusCode());
    }

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
    // response.setHeaders(responseHeaders);

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
    return response;
}
