#include "Config.hpp"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

void applyDefaults(Config &cfg) {
  std::vector<ServerConfig> &servers = cfg.getServers();
  for (size_t i = 0; i < servers.size(); ++i) {
    ServerConfig &srv = servers[i];

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
      LocationConfig &loc = srv.locations[j];

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
      for (std::map<std::string, std::string>::const_iterator it =
               srv.cgi_map.begin();
           it != srv.cgi_map.end(); ++it) {
        if (loc.cgi_map.find(it->first) == loc.cgi_map.end())
          loc.cgi_map[it->first] = it->second;
      }
    }
  }
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
    for (size_t j = 0; j < srv.locations.size(); ++j) {
      const LocationConfig &loc = srv.locations[j];

      // Example: POST on a location that doesn't look like an upload endpoint.
      // (No CGI feature in our minimal structs yet; add a check later if/when
      // CGI exists.)
      if (loc.methods.count(POST) && loc.redirect.status == 0 &&
          loc.root.find("upload") == std::string::npos) {
        std::cerr
            << "Warning: Location " << loc.path
            << " allows POST but is not obviously an upload route (no 'upload' "
               "in root and no redirect). "
               "Consider restricting methods or implementing POST handling.\n";
      }

      // Example: DELETE on static root
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
    }
  }
}
