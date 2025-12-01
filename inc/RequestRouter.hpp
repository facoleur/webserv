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
    bool        resourceExists(const std::string&, const Request&);
    bool        isMethodAllowed(const Request&, const LocationConfig&);
    bool        isCgiRequest(const std::string&, const LocationConfig&);
    std::string getCgiInterpreter(const std::string&, const LocationConfig&) const;
    void handleCgiHeaders(std::string&, std::string&, std::map<std::string, std::string>&, int&, std::string&) const;
    Response    handleGet(const Request&, std::string&, const LocationConfig&);
    Response    handlePost(const Request&, const std::string&, const LocationConfig&);
    Response    handleDelete(const Request&, const std::string&, const LocationConfig&);
    Response    handleCgi(const Request&, const std::string&, const ServerConfig&, const LocationConfig&);
    Response    makeResponse(statusCode);
    Response    makeErrorResponse(statusCode);
    Response    makeRedirectResponse(const std::string&);
    void        resolveAbsolutePath(std::string&);
    std::string getMimeType(const std::string&);
    int executeCgi(const ServerConfig&, const LocationConfig&, const Request&, const std::string&, const std::string&,
                   std::string&, std::map<std::string, std::string>&, int&, std::string&) const;

    std::string generateErrorHtml(enum statusCode);
    std::string generateHtml(const std::string&);
    // std::string generateHtml(const std::string&, const std::map<std::string, std::string>&);

  public:
    Response makeAutoindexResponse(const std::string&, const std::string&);
    RequestRouter();
    ~RequestRouter();

    std::string resolvePath(const Request&, const std::string&, const std::string&);
    Response    route(const Request&, const ServerConfig&);
};

const LocationConfig* findLocationConfig(const std::string& path, const ServerConfig& config);
const LocationConfig  resolveConfig(const ServerConfig& server, const LocationConfig* location);
