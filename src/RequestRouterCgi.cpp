// RequestRouterCgi.cpp

#include <sstream>
#include <unistd.h>

#include "Enums.hpp"
#include "Logger.hpp"
#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include "Utils.hpp"

bool RequestRouter::isCgiRequest(const std::string& path) {
    return !getCgiInterpreter(path).empty();
}

std::string RequestRouter::getCgiInterpreter(const std::string& path) const {
    const std::map<std::string, std::string>& extension = _config.cgiMap;
    if (extension.empty())
        return "";

    std::string::size_type dot = path.find_last_of('.');
    if (dot != std::string::npos) {
        std::string                                        ext = path.substr(dot);
        std::map<std::string, std::string>::const_iterator it  = extension.find(ext);
        if (it != extension.end())
            return it->second;
        if (dot + 1 < path.size()) {
            std::string alt = path.substr(dot + 1);
            it              = extension.find(alt);
            if (it != extension.end())
                return it->second;
            if (!alt.empty()) {
                std::string withDot = "." + alt;
                it                  = extension.find(withDot);
                if (it != extension.end())
                    return it->second;
            }
        }
    }
    return "";
}

std::vector<std::string> RequestRouter::storeCgiEnv(const Request& request, const ServerConfig& serverConfig,
                                                    const std::string& scriptPath) const {
    std::vector<std::string> envStorage;

    std::string protocol      = request.getProtocolVersion().empty() ? "HTTP/1.1" : request.getProtocolVersion();
    std::string host          = request.getHeader(HOST);
    std::string contentType   = request.getHeader(CONTENT_TYPE);
    std::string contentLength = request.getHeader(CONTENT_LENGTH);
    std::string documentRoot  = _config.root.empty() ? serverConfig.root : _config.root;

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
    envStorage.push_back("server_name=" + (serverConfig.host.empty() ? std::string("localhost") : serverConfig.host));

    if (!serverConfig.listenPorts.empty())
        envStorage.push_back("SERVER_PORT=" + toString(serverConfig.listenPorts[0]));
    if (!host.empty())
        envStorage.push_back("HTTP_HOST=" + host);
    if (!contentType.empty())
        envStorage.push_back("CONTENT_TYPE=" + contentType);
    if (!contentLength.empty())
        envStorage.push_back("CONTENT_LENGTH=" + contentLength);

    return envStorage;
}

Response RequestRouter::prepareCgi(Request& req, const std::string& path, const ServerConfig& serverConfig) {

    // check path to script
    if (!isSubPath(_config.root, path))
        return makeErrorResponse(FORBIDDEN);

    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return makeErrorResponse(NOT_FOUND);
    if (!S_ISREG(st.st_mode) || access(path.c_str(), R_OK) != 0)
        return makeErrorResponse(FORBIDDEN);
    req.cgiInfo.setScriptPath(path);

    // set CGI interpreter
    std::string interpreter = getCgiInterpreter(path);
    if (interpreter.empty())
        return makeErrorResponse(BAD_GATEWAY);
    req.cgiInfo.setInterpreter(interpreter);

    // prepare CGI env variables
    std::vector<std::string> envStorage = storeCgiEnv(req, serverConfig, path);
    req.cgiInfo.setEnvStorage(envStorage);

    // set CGI owning request
    req.cgiInfo.setRequest(req);

    Response response;
    response.setMustLaunchCgi(true);
    return response;
}

Response RequestRouter::generateResponseFromCgiOutput(Request& req, Response& response, std::string output) {
    LOG_DEBUG("generateResponseFromCgiOutput()");

    std::map<enum requestHeaders, std::string> responseHeaders;
    std::map<std::string, requestHeaders>      headerStringToEnum;
    std::string                                responseBody;
    int                                        statusCode    = 200;
    std::string                                statusMessage = "OK";

    // check if there was an error
    if (req.getStatusCode() != NO_STATUS) {
        std::string reasonPhrase(ReasonPhrase::get(req.getStatusCode()));
        LOG_DEBUG("RequestRouter::generateResponseFromCgiOutput(): status already set before to: " + reasonPhrase);
        return makeErrorResponse(req.getStatusCode());
    }

    // extract headers and body
    initHeaderStringToEnumMap(headerStringToEnum);
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

    // get headers
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
                    headerValue = toString(statusCode) + statusMessage;
                }
            }
            std::map<std::string, requestHeaders>::const_iterator it = headerStringToEnum.find(lowered);
            if (it == headerStringToEnum.end())
                continue; // unknown header
            else
                responseHeaders[it->second] = headerValue;
        }
    }

    // defaults
    if (!responseHeaders.count(CONTENT_TYPE))
        responseHeaders[CONTENT_TYPE] = "text/html";
    if (!responseHeaders.count(CONTENT_LENGTH))
        responseHeaders[CONTENT_LENGTH] = toString(responseBody.size());

    // set headers in response
    if (responseHeaders.count(CONTENT_TYPE))
        response.setHeader(CONTENT_TYPE, responseHeaders[CONTENT_TYPE]);
    if (responseHeaders.count(CONTENT_LENGTH))
        response.setHeader(CONTENT_LENGTH, responseHeaders[CONTENT_LENGTH]);
    if (responseHeaders.count(HOST))
        response.setHeader(HOST, responseHeaders[HOST]);
    if (responseHeaders.count(LOCATION))
        response.setHeader(LOCATION, responseHeaders[LOCATION]);
    if (responseHeaders.count(TRANSFER_ENCODING))
        response.setHeader(TRANSFER_ENCODING, responseHeaders[TRANSFER_ENCODING]);
    if (responseHeaders.count(SERVER))
        response.setHeader(SERVER, responseHeaders[SERVER]);
    if (responseHeaders.count(ACCEPT))
        response.setHeader(ACCEPT, responseHeaders[ACCEPT]);
    if (responseHeaders.count(DATE))
        response.setHeader(DATE, responseHeaders[DATE]);
    if (responseHeaders.count(CONNECTION))
        response.setHeader(CONNECTION, responseHeaders[CONNECTION]);
    response.setHeaders(responseHeaders);

    // set response statusCode and body
    response.setStatusCode(static_cast<enum statusCode>(statusCode));
    response.setBody(responseBody);

    LOG_DEBUG("generateResponseFromCgiOutput() generated response: ");

    return response;
}
