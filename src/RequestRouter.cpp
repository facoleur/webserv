// RequestRouter.cpp

#include <ctime>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "Config.hpp"
#include "Enums.hpp"
#include "Logger.hpp"
#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include "Utils.hpp"

RequestRouter::RequestRouter() {
}

RequestRouter::~RequestRouter() {
}

bool RequestRouter::resourceExists(const std::string& path) {
    struct stat info;
    return (stat(path.c_str(), &info) == 0);
}

bool RequestRouter::isMethodAllowed(const Request& req, const LocationConfig& config) {
    for (std::set<enum requestMethod>::const_iterator it = config.methods.begin(); it != config.methods.end(); ++it) {
    }

    return (config.methods.find(req.getMethod()) != config.methods.end());
}

void RequestRouter::resolveAbsolutePath(std::string& path) {
    std::string::size_type pos = path.find("http://");
    path.erase(pos, 7);

    pos = path.find("/");
    path.erase(0, pos);
}

std::string extractFile(const std::string& path) {
    if (path == "")
        return std::string("");

    size_t      pos  = path.find_last_of('/');
    std::string file = path.substr(pos);
    return file;
}

std::string RequestRouter::resolvePath(const Request& req, const std::string& root) {
    std::string path = req.getPath();

    if (path.find("%") != std::string::npos || path.find("../") != std::string::npos ||
        path.find("./") != std::string::npos || path.find("=") != std::string::npos ||
        path.find("&") != std::string::npos) {
        throw std::runtime_error("Not accepted in path");
    }

    std::string fsPath = root;

    if (req.getMethod() == POST && _config.uploadEnable) {
        fsPath.append(_config.uploadStore + '/');
    } else {
        fsPath.append(path);
    }

    while (fsPath.find("//") != std::string::npos) {
        replace(fsPath, "//", "/");
    }

    if (fsPath[0] == '/')
        fsPath.erase(fsPath.begin());

    return fsPath;
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

    std::string extension = toLower(path.substr(++pos));
    std::string mimetype  = mime[extension];

    if (mimetype == "")
        return "text/plain";

    return mime[extension];
}

Response RequestRouter::handleGet(const Request& req, std::string& path, const LocationConfig& config) {
    // (void)req;
    Response res;

    if (isDirectory(path)) {

        for (size_t i = 0; i < config.indexFiles.size(); ++i) {
            std::string indexPath = path + config.indexFiles[i];
            if (!resourceExists(indexPath))
                continue;

            std::ifstream file(indexPath.c_str());
            if (!file.is_open())
                continue;

            std::string body = readFile(file);
            res.setHeader(CONTENT_TYPE, getMimeType(indexPath));
            res.setHeader(CONTENT_LENGTH, toString(body.size()));
            res.setBody(body);

            LOG_INFO("Response: " + toString(res.getStatusCode()) + " " + ReasonPhrase::get(res.getStatusCode()));
            return res;
        }

        if (config.autoindex) {
            return makeAutoindexResponse(req.getPath(), path);
        } else {
            return makeErrorResponse(FORBIDDEN);
        }
    }

    if (!resourceExists(path)) {
        return makeErrorResponse(NOT_FOUND);
    }

    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        return makeErrorResponse(NOT_FOUND);
    }

    res.setBody(readFile(file));
    res.setHeader(CONTENT_TYPE, getMimeType(path));
    res.setHeader(CONTENT_LENGTH, toString(res.getBody().size()));

    LOG_INFO("Response: " + toString(res.getStatusCode()) + " " + ReasonPhrase::get(res.getStatusCode()));
    return res;
}

std::string generateUploadName() {
    std::ostringstream oss;
    struct timeval     tv;
    gettimeofday(&tv, 0);
    oss << "upload_" << tv.tv_sec << tv.tv_usec;
    return oss.str();
}

Response RequestRouter::handlePost(const Request& req, const std::string& path, const LocationConfig& config) {

    if (config.uploadEnable == false) {
        return makeErrorResponse(FORBIDDEN);
    }

    std::string basename = path;
    size_t      pos      = path.find_last_of('/');
    if (pos != std::string::npos)
        basename = path.substr(pos + 1);

    std::string fsUploadPath;
    std::string filename = generateUploadName();
    if (!config.uploadStore.empty()) {
        fsUploadPath = config.root + "/" + config.uploadStore + "/" + filename;
    }

    while (fsUploadPath.find("//") != std::string::npos) {
        replace(fsUploadPath, "//", "/");
    }

    std::ofstream out(fsUploadPath.c_str(), std::ios::binary);
    if (!out.is_open()) {
        return makeErrorResponse(INTERNAL_SERVER_ERROR);
    }

    out.write(req.getBody().c_str(), req.getBody().size());

    Response res;

    std::string location = req.getPath() + "/" + filename;
    removeDoubleSlash(location);

    res.setHeader(LOCATION, location);
    res.setStatusCode(CREATED);
    res.setBody(generateHtml("Upload success!"));
    res.setHeader(CONTENT_LENGTH, toString(res.getBody().size()));

    return res;
}

Response RequestRouter::handleDelete(const std::string& path) {
    // if dir -> 403 (design choice, otherwise we must delete recursively dir entries)

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
    replaceVariables(html, "message", ReasonPhrase::get(status));

    return html;
}

std::string RequestRouter::generateHtml(const std::string& message) {
    std::ifstream templateHtml("www/templates/success.html");
    std::string   html = readFile(templateHtml);

    replaceVariables(html, "message", message);

    return html;
}

Response RequestRouter::makeErrorResponse(enum statusCode statusCode) {
    Response res;

    if (statusCode >= 500) {
        LOG_ERROR("Server error: " + toString(statusCode))
    }
    LOG_INFO("Response: " + toString(statusCode) + " " + ReasonPhrase::get(statusCode));

    res.setBody(generateErrorHtml(statusCode));
    res.setHeader(CONTENT_LENGTH, toString(res.getBody().size()));
    res.setStatusCode(statusCode);

    return res;
}

Response RequestRouter::makeAutoindexResponse(const std::string& uri, const std::string& path) {
    DIR* dir = opendir(path.c_str());
    if (!dir)
        return makeErrorResponse(NOT_FOUND);

    std::vector<AutoIndexItem> entries;

    if (uri != "/") {
        std::string parent = getParent(uri);
        entries.push_back(AutoIndexItem(parent, "../", 0, T_DIR, ""));
    }

    while (dirent* entry = readdir(dir)) {

        std::string name(entry->d_name);

        if (name == "." || name == "..")
            continue;

        std::string entryFsPath = path + "/" + entry->d_name;

        struct stat st;
        stat(entryFsPath.c_str(), &st);

        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));

        std::string entryUrl = uri + "/" + entry->d_name;
        removeDoubleSlash(entryUrl);

        bool isDir = false;
        if (entry->d_type == DT_DIR) {
            isDir = true;
            entryUrl.push_back('/');
            name.push_back('/');
        }

        entries.push_back(AutoIndexItem(entryUrl, name, st.st_size, isDir ? T_DIR : T_FILE, timestamp));
    }

    closedir(dir);

    std::string html = AutoIndex::fillTemplate(uri, entries);

    Response res;
    res.setStatusCode(OK);
    res.setHeader(CONTENT_LENGTH, toString(html.size()));
    res.setBody(html);

    LOG_INFO("Response: " + toString(OK) + " " + ReasonPhrase::get(OK));

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
        if (!location->clientMaxBodySize)
            resolved.clientMaxBodySize = server.clientMaxBodySize;

        locationPath       = location->path;
        resolved           = *location;
        resolved.autoindex = location->autoindex;
    } else {
        resolved.autoindex         = server.autoindex;
        resolved.clientMaxBodySize = server.clientMaxBodySize;
    }

    if (resolved.methods.empty()) {
        resolved.methods = server.methods;
    }
    if (resolved.root.empty()) {
        resolved.root = server.root;
    }
    if (resolved.indexFiles.empty()) {
        resolved.indexFiles = server.indexFiles;
    }
    if (resolved.cgiMap.empty()) {
        resolved.cgiMap = server.cgiMap;
    }

    return resolved;
}

Response RequestRouter::makeRedirectResponse(const std::string& location) {
    LOG_INFO("Response: " + toString(REDIRECT) + " " + ReasonPhrase::get(REDIRECT));

    Response res;
    res.setStatusCode(REDIRECT);
    res.setHeader(LOCATION, location);
    res.setHeader(CONTENT_LENGTH, "0");
    res.setBody("");
    return res;
}

Response RequestRouter::route(Request& req, const ServerConfig& config) {

    LOG_INFO("Request:  " + methodToString(req.getMethod()) + " " + req.getPath())

    if (req.getStatusCode() != NO_STATUS) {
        std::string reasonPhrase(ReasonPhrase::get(req.getStatusCode()));
        return makeErrorResponse(req.getStatusCode()); // can be 413 CONTENT_TOO_LARGE, for example
    }

    // dynamic/config-based checks
    const LocationConfig* locationConfig = findLocationConfig(req.getPath(), config);

    std::string           locationPath;
    const LocationConfig& resolvedConfig = resolveConfig(config, locationConfig, locationPath);

    if (req.getBody().size() > resolvedConfig.clientMaxBodySize) {
        return makeErrorResponse(CONTENT_TOO_LARGE);
    }

    _config = resolvedConfig;

    if (resolvedConfig.redirect.status) {
        return makeRedirectResponse(resolvedConfig.redirect.target);
    }

    std::string resolvedPath;

    try {
        resolvedPath = resolvePath(req, resolvedConfig.root);

    } catch (std::exception&) {
        return makeErrorResponse(BAD_REQUEST);
    }

    if (!isMethodAllowed(req, resolvedConfig)) {
        return makeErrorResponse(NOT_ALLOWED);
    }

    if (req.getMethod() == POST && _config.uploadEnable) {
        std::string loc = locationPath;
        if (!loc.empty() && loc[loc.size() - 1] == '/')
            loc.erase(loc.size() - 1);

        std::string p = req.getPath();
        if (!p.empty() && p[p.size() - 1] == '/')
            p.erase(p.size() - 1);

        if (p != loc) {
            return makeErrorResponse(NOT_FOUND);
        }

        if (!isDirectory(resolvedPath)) {
            return makeErrorResponse(NOT_FOUND);
        }

    } else {
        if (!resourceExists(resolvedPath)) {
            return makeErrorResponse(NOT_FOUND);
        }
    }

    if (isDirectory(resolvedPath)) {
        Response res;
        if (!resolvedPath.empty() && resolvedPath[resolvedPath.size() - 1] != '/') {
            res.setStatusCode(REDIRECT);
            res.setHeader(LOCATION, req.getPath() + "/");
            res.setBody("");
            res.setHeader(CONTENT_LENGTH, "0");
            return res;
        }
    }

    if (isCgiRequest(resolvedPath, resolvedConfig))
        return prepareCgi(req, resolvedPath, config, resolvedConfig);

    switch (req.getMethod()) {
        case GET:
            return handleGet(req, resolvedPath, resolvedConfig);
        case POST:
            return handlePost(req, resolvedPath, resolvedConfig);
        case DELETE:
            return handleDelete(resolvedPath);
        default:
            return makeErrorResponse(BAD_REQUEST);
    }
}
