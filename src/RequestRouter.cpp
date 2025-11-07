// RequestRouter.cpp

#include "RequestRouter.hpp"

RequestRouter::RequestRouter() {
}
RequestRouter::~RequestRouter() {
}

bool get_matching_server(ServerConfig& srv, std::string& host) {
    return srv.host == host;
}

#include <string>

// ServerConfig& RequestRouter::match_server(const Request& req) {
//     std::string host = req.getHeader(HOST);

//     if (host == "")
//         return _all_configs.servers[0];

//     DEBUG_LOG("host: " + host);

//     std::vector<ServerConfig>::iterator it = _all_configs.servers.end();
//     for (std::vector<ServerConfig>::iterator it_srv = _all_configs.servers.begin();
//          it_srv != _all_configs.servers.end(); ++it_srv) {
//         if (it_srv->host == host) {
//             it = it_srv;
//             break;
//         }
//     }

//     return *it;
// }

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

void RequestRouter::resolveAbsolutePath(std::string& path) {
    std::string::size_type pos = path.find("http://");
    path.erase(pos, 7);

    pos = path.find("/");
    path.erase(0, pos);
}

std::string RequestRouter::resolvePath(const Request& req, const std::string& root) {
    (void)req;
    // std::string path = req.getPath();
    std::string path = "/";

    if (startsWith(path, "http://"))
        resolveAbsolutePath(path);
    else if (!startsWith(path, "/")) {
        throw std::runtime_error("Relative path oesn't start with /");
    }

    if (path.empty())
        path = "/";

    if (path.find("%") != std::string::npos || path.find("=") != std::string::npos ||
        path.find("../") != std::string::npos || path.find("./") != std::string::npos ||
        path.find("=") != std::string::npos || path.find("&") != std::string::npos)
        throw std::runtime_error("Not accepted in path");

    replace(path, "//", "/");

    std::string fullPath = root;

    fullPath.append(path);

    while (fullPath.find("//") != std::string::npos) {
        replace(fullPath, "//", "/");
    }

    // std::cout << fullPath << std::endl;

    return fullPath;
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

Response RequestRouter::route(const Request& req, const ServerConfig& config) {
    Response response;

    req.printRequest();

    std::string root = config.root;

    std::string fullPath = resolvePath(req, root);

    std::ifstream file(fullPath.c_str());

    if (!file.is_open()) {
        std::cerr << "Failed to open: " << fullPath << std::endl;
        return 1;
    } else if (isDirectory(fullPath)) {
        std::cout << "is directory" << std::endl;
    } else {
        std::cout << "succes open file" << std::endl;
    }

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
