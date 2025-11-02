// RequestRouter.hpp

#include "Request.hpp"
#include "Response.hpp"

class RequestRouter {
  private:
    bool resource_exist(const std::string& path);
    bool is_method_allowed(const Request& req);
    bool is_cgi_request(const std::string& path);

    std::string resolvePath(const Request& req);

    Response handle_get(const Request& req, const std::string& path);
    Response handle_post(const Request& req, const std::string& path);
    Response handle_delete(const Request& req, const std::string& path);
    Response handle_cgi(const Request& req, const std::string& path);

    Response makeErrorResponse(enum statusCode status);

  public:
    RequestRouter();
    ~RequestRouter();

    Response route(const Request& req);
};
