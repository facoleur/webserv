#include "Config.hpp"

#include <Logger.hpp>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#include "ConfigFile.hpp"

void applyDefaults(Config& cfg) {
    std::vector<ServerConfig>& servers = cfg.getServers();
    for (size_t i = 0; i < servers.size(); ++i) {
        ServerConfig& srv = servers[i];

        // Server defaults
        if (srv.root.empty())
            srv.root = "www";
        if (srv.indexFiles.empty())
            srv.indexFiles.push_back("index.html");
        if (srv.methods.empty())
            srv.methods.insert(GET);
        if (srv.clientMaxBodySize == 0)
            srv.clientMaxBodySize = 1048576; // 1 MiB
        // srv.autoindex defaults to false (constructor)

        for (size_t j = 0; j < srv.locations.size(); ++j) {
            LocationConfig& loc = srv.locations[j];

            if (loc.root.empty())
                loc.root = srv.root; // inherit from server
            // if (loc.indexFiles.empty())
            //     loc.indexFiles = srv.indexFiles;
            if (loc.methods.empty())
                loc.methods = srv.methods;
            if (!loc.autoindexSet)
                loc.autoindex = srv.autoindex;
            if (loc.clientMaxBodySize == 0)
                loc.clientMaxBodySize = srv.clientMaxBodySize;
            // Merge CGI maps (server entries fill missing keys)
            for (std::map<std::string, std::string>::const_iterator it = srv.cgiMap.begin(); it != srv.cgiMap.end();
                 ++it) {
                if (loc.cgiMap.find(it->first) == loc.cgiMap.end())
                    loc.cgiMap[it->first] = it->second;
            }
        }
    }
}

static std::string joinPaths(const std::string& root, const std::string& child) {
    if (child.empty())
        return root;
    if (!child.empty() && child[0] == '/')
        return child;
    if (root.empty())
        return child;
    if (root[root.size() - 1] == '/')
        return root + child;
    return root + "/" + child;
}

static void ensureDirectory(const std::string& path, const std::string& context) {
    PathType type = ConfigFile::getTypePath(path);
    if (type != PATH_DIR)
        throw std::runtime_error("Invalid directory for " + context + ": " + path);
    if (ConfigFile::checkFile(path, R_OK | X_OK) != 0)
        throw std::runtime_error("Directory not accessible for " + context + ": " + path);
}

static void ensureIndexFiles(const std::string& root, const std::vector<std::string>& indexes) {
    for (size_t i = 0; i < indexes.size(); ++i) {
        std::string full = joinPaths(root, indexes[i]);
        if (ConfigFile::getTypePath(full) != PATH_FILE || ConfigFile::checkFile(full, R_OK) != 0) {
            throw std::runtime_error("Index file not accessible: " + full);
        }
    }
}

static void ensureCgiMap(const std::map<std::string, std::string>& cgiMap) {
    for (std::map<std::string, std::string>::const_iterator it = cgiMap.begin(); it != cgiMap.end(); ++it) {
        const std::string& interp = it->second;
        if (ConfigFile::getTypePath(interp) != PATH_FILE || ConfigFile::checkFile(interp, X_OK) != 0) {
            throw std::runtime_error("CGI interpreter not executable: " + interp);
        }
    }
}

static std::string parentDir(const std::string& path) {
    std::string::size_type pos = path.find_last_of('/');
    if (pos == std::string::npos)
        return ".";
    if (pos == 0)
        return "/";
    return path.substr(0, pos);
}

static void ensureUploadStore(const LocationConfig& loc, const std::string& root) {
    if (!loc.uploadEnable)
        return;
    if (loc.uploadStore.empty())
        throw std::runtime_error("upload_store required when upload_enable is on");
    std::string path = loc.root.empty() ? root + "/" + loc.uploadStore : loc.root + "/" + loc.uploadStore;
    PathType    type = ConfigFile::getTypePath(loc.root + "/" + loc.uploadStore);
    if (type == PATH_DIR) {
        if (ConfigFile::checkFile(path, W_OK | X_OK) != 0)
            throw std::runtime_error("Upload directory not writable: " + loc.uploadStore);
        return;
    }
    if (type == PATH_ERROR) {
        std::string parent = parentDir(path);
        if (ConfigFile::checkFile(parent, W_OK | X_OK) != 0)
            throw std::runtime_error("Cannot create upload directory (permission "
                                     "denied): " +
                                     path);
        return;
    }
    throw std::runtime_error("Upload store must be a directory: " + path);
}

void validateAmbigousServerBlock(const std::vector<ServerBlock>& serverBlock) {
    for (size_t i = 0; i < serverBlock.size(); i++) {
        const ServerBlock& current = serverBlock[i];

        for (size_t j = 0; j < serverBlock.size(); j++) {
            if (i == j)
                continue;

            const ServerBlock& existing = serverBlock[j];

            if (current == existing)
                throw std::runtime_error("invalid config: duplicate <ip>:<port> server block");
        }
    }
}

void validateCompatibility(const Config& cfg) {
    const std::vector<ServerConfig>& servers = cfg.getServers();

    std::vector<ServerBlock> serverBlock;

    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig& srv = servers[i];

        serverBlock.push_back(ServerBlock(srv.listenPorts, srv.host, srv.serverName));

        // Server-level checks
        if (srv.listenPorts.empty()) {
            std::cerr << "Error: server[" << i << "] missing listen directive (at least one port required)\n";
            throw std::runtime_error("invalid config: missing listen");
        }
        for (size_t p = 0; p < srv.listenPorts.size(); ++p) {
            int port = srv.listenPorts[p];
            if (port < 1 || port > 65535)
                throw std::runtime_error("invalid listen port outside range 1..65535");
        }
        if (srv.clientMaxBodySize == 0)
            throw std::runtime_error("invalid server clientMaxBodySize (must be > 0)");
        for (std::map<int, std::string>::const_iterator it = srv.errorPages.begin(); it != srv.errorPages.end(); ++it) {
            int code = it->first;
            if (code < 100 || code > 599)
                throw std::runtime_error("invalid errorPage code (must be 100..599)");
        }
        ensureDirectory(srv.root, "server root");
        ensureIndexFiles(srv.root, srv.indexFiles);
        ensureCgiMap(srv.cgiMap);
        for (size_t j = 0; j < srv.locations.size(); ++j) {
            const LocationConfig& loc = srv.locations[j];
            // if (loc.methods.count(POST) && loc.redirect.status == 0 && loc.root.find("upload") ==
            // std::string::npos)
            // {
            //     std::cerr << "Warning: Location " << loc.path
            //               << " allows POST but is not obviously an upload route (no 'upload' "
            //                  "in root and no redirect). "
            //                  "Consider restricting methods or implementing POST handling.\n";
            // }
            if (loc.methods.count(DELETE) && loc.redirect.status != 0) {
                std::cerr << "Warning: Location " << loc.path << " defines DELETE but has a redirect\n";
            }
            if (loc.clientMaxBodySize == 0)
                throw std::runtime_error("invalid location client_max_body_size (must be > 0)");
            if (loc.uploadEnable && loc.uploadStore.empty())
                throw std::runtime_error("upload enabled but upload_store not set in location");
            ensureDirectory(loc.root, "location root");
            ensureIndexFiles(loc.root, loc.indexFiles);
            ensureCgiMap(loc.cgiMap);
            ensureUploadStore(loc, srv.root);
        }
    }

    validateAmbigousServerBlock(serverBlock);
}

bool ServerConfig::matchServerName(const std::string& hostHeader) const {
    if (hostHeader.find(serverName) != std::string::npos)
        return true;
    return false;
}
