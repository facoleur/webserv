// RequestRouter.hpp

#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include <algorithm>

class RequestRouter {
  private:
    Config       _all_configs;
    ServerConfig _config;

    bool resource_exist(const std::string& path);
    bool is_method_allowed(const Request& req);
    bool is_cgi_request(const std::string& path);

    ServerConfig& match_server(const Request& req);
    std::string   resolvePath(const Request& req);

    Response handle_get(const Request& req, const std::string& path);
    Response handle_post(const Request& req, const std::string& path);
    Response handle_delete(const Request& req, const std::string& path);
    Response handle_cgi(const Request& req, const std::string& path);
    Response make_error_response(int status_code);

  public:
    RequestRouter();
    ~RequestRouter();

    Response route(const Request& req);
};
