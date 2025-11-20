// RequestRouter.hpp

#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <fstream>

class RequestRouter {
  private:
    bool        resourceExist(const std::string&);
    bool        isMethodAllowed(const Request&, const LocationConfig&);
    bool        isCgiRequest(const std::string&, const LocationConfig&);
    std::string getCgiInterpreter(const std::string&, const LocationConfig&) const;
    std::string readFile(const std::ifstream&);
    Response    handleGet(const Request&, std::string&, const ServerConfig&);
    Response    handlePost(const Request&, const std::string&);
    Response    handleDelete(const Request&, const std::string&);
    Response    handleCgi(const Request&, const std::string&, const ServerConfig&, const LocationConfig&);
    Response    makeErrorResponse(enum statusCode);
    Response    makeAutoindexResponse(const std::string&);
    Response    makeRedirectResponse(const std::string&);
    void        resolveAbsolutePath(std::string&);
    std::string getMimeType(const std::string&);
    int         executeCgi(const ServerConfig&, const LocationConfig&, const Request&, const std::string&,
                           const std::string&, std::string&, std::map<std::string, std::string>&, int&,
                           std::string&) const;

  public:
    RequestRouter();
    ~RequestRouter();

    std::string resolvePath(const Request&, const std::string&);
    Response    route(const Request&, const ServerConfig&);
};

const LocationConfig* findLocationConfig(const std::string& path, const ServerConfig& config);
const LocationConfig  resolveConfig(const ServerConfig& server, const LocationConfig* location);
