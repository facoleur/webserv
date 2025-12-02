#pragma once

// RequestRouter.hpp

#include "AutoIndex.hpp"
#include "Config.hpp"
#include "Enums.hpp"
#include "Utils.hpp"

class Request;
class Response;

class RequestRouter {
  protected:
    bool resourceExists(const std::string&, const Request&);
    bool isMethodAllowed(const Request&, const LocationConfig&);

    Response handleGet(const Request&, std::string&, const LocationConfig&);
    Response handlePost(const Request&, const std::string&, const LocationConfig&);
    Response handleDelete(const Request&, const std::string&, const LocationConfig&);

    Response    makeResponse(statusCode);
    Response    makeErrorResponse(statusCode);
    Response    makeRedirectResponse(const std::string&);
    void        resolveAbsolutePath(std::string&);
    std::string getMimeType(const std::string&);

    std::string generateErrorHtml(enum statusCode);
    std::string generateHtml(const std::string&);
    // std::string generateHtml(const std::string&, const std::map<std::string, std::string>&);

    // CGI preparation
    bool                     isCgiRequest(const std::string&, const LocationConfig&);
    std::string              getCgiInterpreter(const std::string&, const LocationConfig&) const;
    std::vector<std::string> storeCgiEnv(const Request& request, const LocationConfig& locationConfig,
                                         const ServerConfig& serverConfig, const std::string& scriptPath) const;
    Response                 prepareCgi(Request&, const std::string&, const ServerConfig&, const LocationConfig&);

  public:
    Response makeAutoindexResponse(const std::string&, const std::string&);
    RequestRouter();
    ~RequestRouter();

    std::string resolvePath(const Request&, const std::string&, const std::string&);
    Response    route(Request&, const ServerConfig&);
};

const LocationConfig* findLocationConfig(const std::string& path, const ServerConfig& config);
const LocationConfig  resolveConfig(const ServerConfig& server, const LocationConfig* location);
