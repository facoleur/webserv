// RequestRouter.hpp

#include "AutoIndex.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"
#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>

class RequestRouter {
  protected:
    bool        resourceExist(const std::string&, const Request&);
    bool        isMethodAllowed(const Request&, const LocationConfig&);
    bool        isCgiRequest(const std::string&, const LocationConfig&);
    Response    handleGet(const Request&, std::string&, const LocationConfig&);
    Response    handlePost(const Request&, const std::string&, const LocationConfig&);
    Response    handleDelete(const Request&, const std::string&, const LocationConfig&);
    Response    handleCgi(const Request&, const std::string&, const LocationConfig&);
    Response    makeResponse(enum statusCode statusCode);
    Response    makeErrorResponse(enum statusCode);
    Response    makeRedirectResponse(const std::string&);
    void        resolveAbsolutePath(std::string&);
    std::string getMimeType(const std::string&);

  public:
    Response makeAutoindexResponse(const std::string&);
    RequestRouter();
    ~RequestRouter();

    std::string resolvePath(const Request&, const std::string&, const std::string&);
    Response    route(const Request&, const ServerConfig&);
};
