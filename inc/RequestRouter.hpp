// RequestRouter.hpp

#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <dirent.h>
#include <fstream>

class RequestRouter {
  private:
    bool        resourceExist(const std::string&);
    bool        isMethodAllowed(const Request&, const LocationConfig&);
    bool        isCgiRequest(const std::string&, const LocationConfig&);
    std::string readFile(const std::ifstream&);
    Response    handleGet(const Request&, std::string&, const ServerConfig&);
    Response    handlePost(const Request&, const std::string&);
    Response    handleDelete(const Request&, const std::string&);
    Response    handleCgi(const Request&, const std::string&);
    Response    makeErrorResponse(enum statusCode);
    Response    makeRedirectResponse(const std::string&);
    void        resolveAbsolutePath(std::string&);
    std::string getMimeType(const std::string&);

  public:
    Response makeAutoindexResponse(const std::string&);
    RequestRouter();
    ~RequestRouter();

    std::string resolvePath(const Request&, const std::string&);
    Response    route(const Request&, const ServerConfig&);
};
