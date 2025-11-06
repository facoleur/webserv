// RequestRouter.cpp

#include "RequestRouter.hpp"

RequestRouter::RequestRouter() {
}
RequestRouter::~RequestRouter() {
}

bool get_matching_server(ServerConfig& srv, std::string& host) {
    return srv.host == host;
}

ServerConfig& RequestRouter::match_server(const Request& req) {
    std::string host = req.getHeader(HOST);

    if (host == "")
        return _all_configs.servers[0];

    DEBUG_LOG("host: " + host);

    std::vector<ServerConfig>::iterator it = _all_configs.servers.end();
    for (std::vector<ServerConfig>::iterator it_srv = _all_configs.servers.begin();
         it_srv != _all_configs.servers.end(); ++it_srv) {
        if (it_srv->host == host) {
            it = it_srv;
            break;
        }
    }

    return *it;
}

bool RequestRouter::resource_exist(const std::string& path) {
    (void)path;
    return true;
}

bool RequestRouter::is_method_allowed(const Request& req) {
    (void)req;
    return true;
}

bool RequestRouter::is_cgi_request(const std::string& path) {
    (void)path;
    return true;
}

std::string RequestRouter::resolvePath(const Request& req) {
    std::string full_path;

    full_path.append(_config.host);
    full_path.append(req.getPath());

    std::cout << full_path << std::endl;

    return full_path;
}

Response RequestRouter::handle_get(const Request& req, const std::string& path) {
    (void)req;
    (void)path;
    Response res;
    return res;
}
Response RequestRouter::handle_post(const Request& req, const std::string& path) {
    (void)req;
    (void)path;
    Response res;
    return res;
}
Response RequestRouter::handle_delete(const Request& req, const std::string& path) {
    (void)req;
    (void)path;
    Response res;
    return res;
}
Response RequestRouter::handle_cgi(const Request& req, const std::string& path) {
    (void)req;
    (void)path;
    Response res;
    return res;
}

Response RequestRouter::make_error_response(int status_code) {
    (void)status_code;
    Response res;
    return res;
}

bool is_same_server(const ServerConfig& server, std::string& host) {
    return server.host == host;
}

Response RequestRouter::route(const Request& req) {
    Response response;

    _config = match_server(req);
    std::cout << "ICIOK" << std::endl;
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
