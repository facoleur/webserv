#include "Config.hpp"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

#include "ConfigFile.hpp"

void applyDefaults(Config& cfg) {
    std::vector<ServerConfig>& servers = cfg.getServers();
    for (size_t i = 0; i < servers.size(); ++i) {
        ServerConfig& srv = servers[i];

        // Server defaults
        if (srv.root.empty())
            srv.root = "./www";
        if (srv.index_files.empty())
            srv.index_files.push_back("index.html");
        if (srv.methods.empty())
            srv.methods.insert(GET);
        if (srv.client_max_body_size == 0)
            srv.client_max_body_size = 1048576; // 1 MiB
        // srv.autoindex defaults to false (constructor)

        for (size_t j = 0; j < srv.locations.size(); ++j) {
            LocationConfig& loc = srv.locations[j];

            if (loc.root.empty())
                loc.root = srv.root; // inherit from server
            if (loc.index_files.empty())
                loc.index_files = srv.index_files;
            if (loc.methods.empty())
                loc.methods = srv.methods;
            if (!loc.autoindex_set)
                loc.autoindex = srv.autoindex;
            if (loc.client_max_body_size == 0)
                loc.client_max_body_size = srv.client_max_body_size;
            // Merge CGI maps (server entries fill missing keys)
            for (std::map<std::string, std::string>::const_iterator it = srv.cgi_map.begin(); it != srv.cgi_map.end();
                 ++it) {
                if (loc.cgi_map.find(it->first) == loc.cgi_map.end())
                    loc.cgi_map[it->first] = it->second;
            }
        }
    }
}

static std::string joinPaths(const std::string &root, const std::string &child) {
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

static void ensureDirectory(const std::string &path,
                            const std::string &context) {
  PathType type = ConfigFile::getTypePath(path);
  if (type != PATH_DIR)
    throw std::runtime_error("Invalid directory for " + context + ": " + path);
  if (ConfigFile::checkFile(path, R_OK | X_OK) != 0)
    throw std::runtime_error("Directory not accessible for " + context + ": " +
                             path);
}

static void ensureIndexFiles(const std::string &root,
                             const std::vector<std::string> &indexes) {
  for (size_t i = 0; i < indexes.size(); ++i) {
    std::string full = joinPaths(root, indexes[i]);
    if (ConfigFile::getTypePath(full) != PATH_FILE ||
        ConfigFile::checkFile(full, R_OK) != 0) {
      throw std::runtime_error("Index file not accessible: " + full);
    }
  }
}

static void ensureCgiMap(const std::map<std::string, std::string> &cgi_map) {
  for (std::map<std::string, std::string>::const_iterator it = cgi_map.begin();
       it != cgi_map.end(); ++it) {
    const std::string &interp = it->second;
    if (ConfigFile::getTypePath(interp) != PATH_FILE ||
        ConfigFile::checkFile(interp, X_OK) != 0) {
      throw std::runtime_error("CGI interpreter not executable: " + interp);
    }
  }
}

static std::string parentDir(const std::string &path) {
  std::string::size_type pos = path.find_last_of('/');
  if (pos == std::string::npos)
    return ".";
  if (pos == 0)
    return "/";
  return path.substr(0, pos);
}

static void ensureUploadStore(const LocationConfig &loc) {
  if (!loc.upload_enable)
    return;
  if (loc.upload_store.empty())
    throw std::runtime_error("upload_store required when upload_enable is on");
  PathType type = ConfigFile::getTypePath(loc.upload_store);
  if (type == PATH_DIR) {
    if (ConfigFile::checkFile(loc.upload_store, W_OK | X_OK) != 0)
      throw std::runtime_error("Upload directory not writable: " +
                               loc.upload_store);
    return;
  }
  if (type == PATH_ERROR) {
    std::string parent = parentDir(loc.upload_store);
    if (ConfigFile::checkFile(parent, W_OK | X_OK) != 0)
      throw std::runtime_error("Cannot create upload directory (permission "
                               "denied): " +
                               loc.upload_store);
    return;
  }
  throw std::runtime_error("Upload store must be a directory: " +
                           loc.upload_store);
}

void validateCompatibility(const Config &cfg) {
  const std::vector<ServerConfig> &servers = cfg.getServers();
  for (size_t i = 0; i < servers.size(); ++i) {
    const ServerConfig &srv = servers[i];
    // Server-level checks
    if (srv.listen_ports.empty()) {
      std::cerr << "Error: server[" << i
                << "] missing listen directive (at least one port required)\n";
      throw std::runtime_error("invalid config: missing listen");
    }
    for (size_t p = 0; p < srv.listen_ports.size(); ++p) {
      int port = srv.listen_ports[p];
      if (port < 1 || port > 65535)
        throw std::runtime_error("invalid listen port outside range 1..65535");
    }
    if (srv.client_max_body_size == 0)
      throw std::runtime_error("invalid server client_max_body_size (must be > 0)");
    for (std::map<int, std::string>::const_iterator it = srv.error_pages.begin();
         it != srv.error_pages.end(); ++it) {
      int code = it->first;
      if (code < 100 || code > 599)
        throw std::runtime_error("invalid error_page code (must be 100..599)");
    }
    ensureDirectory(srv.root, "server root");
    ensureIndexFiles(srv.root, srv.index_files);
    ensureCgiMap(srv.cgi_map);
    for (size_t j = 0; j < srv.locations.size(); ++j) {
      const LocationConfig &loc = srv.locations[j];
      if (loc.methods.count(POST) && loc.redirect.status == 0 &&
          loc.root.find("upload") == std::string::npos) {
        std::cerr
            << "Warning: Location " << loc.path
            << " allows POST but is not obviously an upload route (no 'upload' "
               "in root and no redirect). "
               "Consider restricting methods or implementing POST handling.\n";
      }
      if (loc.methods.count(DELETE) && loc.redirect.status != 0) {
        std::cerr << "Warning: Location " << loc.path
                  << " defines DELETE but has a redirect\n";
      }
      if (loc.client_max_body_size == 0)
        throw std::runtime_error(
            "invalid location client_max_body_size (must be > 0)");
      if (loc.upload_enable && loc.upload_store.empty())
        throw std::runtime_error(
            "upload enabled but upload_store not set in location");
      ensureDirectory(loc.root, "location root");
      ensureIndexFiles(loc.root, loc.index_files);
      ensureCgiMap(loc.cgi_map);
      ensureUploadStore(loc);
    }
  }
}
