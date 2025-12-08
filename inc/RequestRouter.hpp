#pragma once

// RequestRouter.hpp

#include "AutoIndex.hpp"
#include "Config.hpp"
#include "Enums.hpp"
#include "Server.hpp"

class Request;
class Response;

class RequestRouter {

  protected:
    LocationConfig           _config;
    bool                     resourceExists(const std::string&);
    bool                     isMethodAllowed(const Request&);
    bool                     isCgiRequest(const std::string&);
    std::string              getCgiInterpreter(const std::string&) const;
    Response                 handleGet(const Request&, std::string&);
    Response                 handlePost(const Request&, const std::string&);
    Response                 handleDelete(const std::string&);
    Response                 prepareCgi(Request&, const std::string&, const ServerConfig&);
    std::vector<std::string> storeCgiEnv(const Request& request, const ServerConfig& serverConfig,
                                         const std::string& scriptPath) const;
    Response                 makeResponse(statusCode);
    Response                 makeErrorResponse(statusCode);
    Response                 makeRedirectResponse(const std::string&);
    void                     resolveAbsolutePath(std::string&);
    std::string              getMimeType(const std::string&);

    int executeCgi(const ServerConfig&, const Request&, const std::string&, const std::string&, std::string&,
                   std::map<std::string, std::string>&, int&, std::string&) const;

    std::string generateErrorHtml(enum statusCode);
    std::string generateHtml(const std::string&);

  public:
    Response makeAutoindexResponse(const std::string&, const std::string&);
    RequestRouter();
    ~RequestRouter();

    std::string resolvePath(const Request&, const std::string&);
    Response    route(Request&, const ServerConfig&);
    Response    generateResponseFromCgiOutput(Request&, Response&, std::string);
};

const LocationConfig* findLocationConfig(const std::string& path, const ServerConfig& config);
const LocationConfig  resolveConfig(const ServerConfig& server, const LocationConfig* location,
                                    std::string& locationPath);
