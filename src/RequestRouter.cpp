// RequestRouter.cpp

#include "RequestRouter.hpp"

bool resource_exist(const std::string& path) {
}
bool is_method_allowed(const Request& req) {
}
std::string resolvePath(const Request& req) {
}

Response handle_get(const Request& req, const std::string& path) {
}
Response handle_post(const Request& req, const std::string& path) {
}
Response handle_delete(const Request& req, const std::string& path) {
}
Response handle_cgi(const Request& req, const std::string& path) {
}

Response RequestRouter::route(const Request& req) {
    Response    response;
    std::string fullPath = resolvePath(req);

    if (!resource_exist(fullPath))
        return make_error_response(404);

    if (!is_method_allowed(req))
        return make_error_response(405);

    if (is_cgi_request(fullPath))
        return handle_cgi(req, fullPath);

    switch (req.getMethod()) {
        case GET:
            return handle_get(req, fullPath);
        case POST:
            return handle_post(req, fullPath);
        case DELETE:
            return handle_delete(req, fullPath);
        default:
            return make_error_response(400);
    }
}
