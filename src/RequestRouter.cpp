// RequestRouter.cpp

#include <cctype>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "Enums.hpp"
#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

RequestRouter::RequestRouter() {
}

RequestRouter::~RequestRouter() {
}

bool RequestRouter::resourceExists(const std::string& path, const Request& req) {
    (void)req;
    struct stat info;
    return (stat(path.c_str(), &info) == 0);
}

bool RequestRouter::isMethodAllowed(const Request& req, const LocationConfig& config) {
    for (std::set<enum requestMethod>::const_iterator it = config.methods.begin(); it != config.methods.end(); ++it) {
    }

    return (config.methods.find(req.getMethod()) != config.methods.end());
}

bool RequestRouter::isCgiRequest(const std::string& path, const LocationConfig& config) {
    return !getCgiInterpreter(path, config).empty();
}

std::string RequestRouter::getCgiInterpreter(const std::string& path, const LocationConfig& config) const {
    const std::map<std::string, std::string>& cgi_ext = config.cgi_map;
    if (cgi_ext.empty())
        return "";

    std::string::size_type dot = path.find_last_of('.');
    if (dot != std::string::npos) {
        std::string                                        ext = path.substr(dot);
        std::map<std::string, std::string>::const_iterator it  = cgi_ext.find(ext);
        if (it != cgi_ext.end())
            return it->second;
        if (dot + 1 < path.size()) {
            std::string alt = path.substr(dot + 1);
            it              = cgi_ext.find(alt);
            if (it != cgi_ext.end())
                return it->second;
            if (!alt.empty()) {
                std::string withDot = "." + alt;
                it                  = cgi_ext.find(withDot);
                if (it != cgi_ext.end())
                    return it->second;
            }
        }
    }
    return "";
}

void RequestRouter::resolveAbsolutePath(std::string& path) {
    std::string::size_type pos = path.find("http://");
    path.erase(pos, 7);

    pos = path.find("/");
    path.erase(0, pos);
}

std::string RequestRouter::resolvePath(const Request& req, const std::string& root, const std::string& location) {
    (void)req;
    std::string path = req.getPath();
    // std::cout << "path: " << path << std::endl;
    // std::cout << "location: " << location << std::endl;

    path = "/" + path.substr(location.size());

    // if (startsWith(path, "http://"))
    //     resolveAbsolutePath(path);
    // else if (!startsWith(path, "/")) {
    //     throw std::runtime_error("Relative path doesn't start with /");
    // }

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

    if (fullPath[0] == '/')
        fullPath.erase(fullPath.begin());

    return fullPath;
}

std::string RequestRouter::getMimeType(const std::string& path) {
    std::map<std::string, std::string> mime;
    mime["html"] = "text/html";
    mime["htm"]  = "text/html";
    mime["css"]  = "text/css";
    mime["js"]   = "application/javascript";
    mime["jpg"]  = "image/jpeg";
    mime["jpeg"] = "image/jpeg";
    mime["png"]  = "image/png";
    mime["gif"]  = "image/gif";

    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos)
        return "text/plain";

    std::string extension = tolower(path.substr(++pos));
    std::string mimetype  = mime[extension];

    if (mimetype == "")
        return "text/plain";

    return mime[extension];
}

Response RequestRouter::handleGet(const Request& req, std::string& path, const LocationConfig& config) {
    (void)req;
    Response res;

    DEBUG_LOG("HANDLING GET");

    if (isDirectory(path)) {

        if (!path.empty() && path[path.size() - 1] != '/') {
            res.setStatusCode(REDIRECT);
            res.setHeader(LOCATION, path + "/");
            res.setBody("");
            res.setHeader(CONTENT_LENGTH, "0");
            DEBUG_LOG("GET DONE redirect");

            return res;
        }

        for (size_t i = 0; i < config.index_files.size(); ++i) {
            std::string indexPath = path + config.index_files[i];
            if (!resourceExists(indexPath, req))
                continue;

            std::ifstream file(indexPath.c_str());
            if (!file.is_open())
                continue;

            std::string body = readFile(file);
            res.setHeader(CONTENT_TYPE, getMimeType(indexPath));
            res.setHeader(CONTENT_LENGTH, toString(body.size()));
            res.setBody(body);
            DEBUG_LOG("GET DONE inedxfile");

            return res;
        }

        if (config.autoindex) {
            DEBUG_LOG("GET DONE autoindex");
            return makeAutoindexResponse(path, config.path);
        } else {
            DEBUG_LOG("GET DONE 403 no autoindex");
            return makeErrorResponse(FORBIDDEN);
        }
    }

    if (!resourceExists(path, req)) {
        DEBUG_LOG("GET DONE 404");
        return makeErrorResponse(NOT_FOUND);
    }

    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        DEBUG_LOG("GET DONE cant open fine");
        return makeErrorResponse(NOT_FOUND);
    }

    res.setBody(readFile(file));
    res.setHeader(CONTENT_TYPE, getMimeType(path));
    res.setHeader(CONTENT_LENGTH, toString(res.getBody().size()));

    DEBUG_LOG("GET DONE good");

    return res;
}

std::string generateUploadName() {
    std::ostringstream oss;
    oss << "upload_" << std::time(0);
    return oss.str();
}

Response RequestRouter::handlePost(const Request& req, const std::string& path, const LocationConfig& config) {
    DEBUG_LOG("handlePOST");

    if (config.upload_enable == false) {
        return makeErrorResponse(FORBIDDEN);
    }

    std::string basename = path;
    size_t      pos      = path.find_last_of('/');
    if (pos != std::string::npos)
        basename = path.substr(pos + 1);

    std::string uploadPath;
    std::string filename = generateUploadName();
    if (!config.upload_store.empty())
        uploadPath = config.root + "/" + config.upload_store + "/" + filename;

    while (uploadPath.find("//") != std::string::npos) {
        replace(uploadPath, "//", "/");
    }

    std::cout << "uploadPath: " << uploadPath << std::endl;
    std::ofstream out(uploadPath.c_str(), std::ios::binary);
    if (!out.is_open()) {
        std::cout << "500 ici" << std::endl;
        return makeErrorResponse(INTERNAL_SERVER_ERROR);
    }

    out.write(req.getBody().c_str(), req.getBody().size());

    Response res;
    res.setHeader(LOCATION, uploadPath);
    res.setStatusCode(CREATED);
    res.setBody(generateHtml("Upload success!"));
    res.setHeader(CONTENT_LENGTH, toString(res.getBody().size()));

    return res;
}

Response RequestRouter::handleDelete(const Request& req, const std::string& path, const LocationConfig& config) {
    // if dir -> 403 (design choice, otherwise we must delete recursively dir entries)
    (void)req;
    (void)config;

    if (isDirectory(path)) {
        return makeErrorResponse(FORBIDDEN);
    }

    if (access(path.c_str(), W_OK)) {
        return makeErrorResponse(FORBIDDEN);
    }

    if (std::remove(path.c_str()) != 0) {
        return makeErrorResponse(INTERNAL_SERVER_ERROR);
    }

    return makeResponse(NO_CONTENT);
}

Response RequestRouter::makeResponse(enum statusCode statusCode) {
    Response res;
    res.setStatusCode(statusCode);

    return res;
}

std::string RequestRouter::generateErrorHtml(enum statusCode status) {
    std::ifstream templateHtml("www/templates/error.html");
    std::string   html = readFile(templateHtml);

    replaceVariables(html, "statusCode", toString(status));
    replaceVariables(html, "message", toString(status));

    return html;
}

// std::string RequestRouter::generateHtml(const std::string&                        templatePath,
//                                         const std::map<std::string, std::string>& variables) {
//     std::ifstream templateHtml(templatePath);
//     std::string   html = readFile(templateHtml);

//     std::map<std::string, std::string>::const_iterator it;
//     for (it = variables.begin(); it != variables.end(); ++it) {
//         replaceVariables(html, it->first, it->second);
//     }

//     return html;
// }

std::string RequestRouter::generateHtml(const std::string& message) {
    std::ifstream templateHtml("www/templates/success.html");
    std::string   html = readFile(templateHtml);

    replaceVariables(html, "message", message);

    return html;
}

Response RequestRouter::makeErrorResponse(enum statusCode statusCode) {
    Response res;
    res.setBody(generateErrorHtml(statusCode));
    res.setStatusCode(statusCode);

    return res;
}

Response RequestRouter::makeAutoindexResponse(const std::string& path, const std::string& location) {
    DIR* dirstream = opendir(path.c_str());

    std::string loc = location;
    if (loc[loc.size() - 1] != '/')
        loc.push_back('/');

    std::vector<AutoIndexItem> files;
    files.push_back(AutoIndexItem(getParentDir(path), "../", 0, T_DIR, ""));

    while (dirent* f = readdir(dirstream)) {

        if (std::string(f->d_name) == "." || std::string(f->d_name) == "..")
            continue;

        std::string name(f->d_name);
        std::string fileloc = location + "/" + f->d_name;

        std::string fullpath = path + "/" + f->d_name;
        struct stat st;
        stat(fullpath.c_str(), &st);

        time_t     lastModified = st.st_mtime;
        char       lastModifedReadable[64];
        struct tm* tm = localtime(&lastModified);
        strftime(lastModifedReadable, sizeof(lastModifedReadable), "%Y-%m-%d %H:%M:%S", tm);

        enum autoIndexType type = T_FILE;
        if (f->d_type == DT_DIR) {
            type = T_DIR;
            name.push_back('/');
        }
        files.push_back(AutoIndexItem(fileloc, name, st.st_size, type, lastModifedReadable));
    }

    std::string html = AutoIndex::fillTemplate(loc, files);

    Response res;
    res.setStatusCode(OK);
    res.setHeader(CONTENT_LENGTH, toString(html.size()));
    res.setBody(html);

    return res;
}

const LocationConfig* findLocationConfig(const std::string& path, const ServerConfig& config) {
    const LocationConfig* best    = NULL;
    size_t                bestLen = 0;
    for (std::vector<LocationConfig>::const_iterator it = config.locations.begin(); it != config.locations.end();
         ++it) {
        if (path.compare(0, it->path.size(), it->path) == 0 && it->path.size() > bestLen) {

            best    = &(*it);
            bestLen = it->path.size();
        }
    }
    return best;
}

const LocationConfig resolveConfig(const ServerConfig& server, const LocationConfig* location,
                                   std::string& locationPath) {

    LocationConfig resolved;

    if (location) {
        locationPath = location->path;
        resolved     = *location;
    }

    if (resolved.methods.empty()) {
        resolved.methods = server.methods;
    }
    if (resolved.root.empty()) {
        resolved.root = server.root;
    }
    if (resolved.index_files.empty()) {
        resolved.index_files = server.index_files;
    }
    if (resolved.cgi_map.empty()) {
        resolved.cgi_map = server.cgi_map;
    }

    return resolved;
}

Response RequestRouter::makeRedirectResponse(const std::string& location) {
    Response res;
    res.setStatusCode(REDIRECT);
    res.setHeader(LOCATION, location);
    res.setHeader(CONTENT_LENGTH, "0");
    res.setBody("");
    return res;
}

Response RequestRouter::route(const Request& req, const ServerConfig& config) {
    DEBUG_LOG("RequestRouter.route():");

    if (req.getStatusCode() != NO_STATUS) {
        std::string reasonPhrase(ReasonPhrase::get(req.getStatusCode()));
        DEBUG_LOG("RequestRouter.route(): status already set before to: " + reasonPhrase);
        return makeErrorResponse(req.getStatusCode()); // can be 413 CONTENT_TOO_LARGE, for example
    }

    // dynamic/config-based checks
    const LocationConfig* locationConfig = findLocationConfig(req.getPath(), config);

    std::string           locationPath;
    const LocationConfig& resolvedConfig = resolveConfig(config, locationConfig, locationPath);

    if (resolvedConfig.redirect.status) {
        return makeRedirectResponse(resolvedConfig.redirect.target);
    }

    std::string resolvedPath;

    try {
        resolvedPath = resolvePath(req, resolvedConfig.root, locationPath);
    } catch (std::exception&) {
        return makeErrorResponse(BAD_REQUEST);
    }

    if (!resourceExists(resolvedPath, req)) {
        return makeErrorResponse(NOT_FOUND);
    }

    if (!isMethodAllowed(req, resolvedConfig)) {
        return makeErrorResponse(NOT_ALLOWED);
    }

    if (isCgiRequest(resolvedPath, resolvedConfig))
        return handleCgi(req, resolvedPath, config, resolvedConfig);

    switch (req.getMethod()) {
        case GET:
            return handleGet(req, resolvedPath, resolvedConfig);
        case POST:
            return handlePost(req, resolvedPath, resolvedConfig);
        case DELETE:
            return handleDelete(req, resolvedPath, resolvedConfig);
        default:
            return makeErrorResponse(BAD_REQUEST);
    }
}
