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

    if (srv.root.empty())
      srv.root = "./www";
    if (srv.index_files.empty())
      srv.index_files.push_back("index.html");
    if (srv.methods.empty())
      srv.methods.insert("GET");

    for (size_t j = 0; j < srv.locations.size(); ++j) {
      LocationConfig &loc = srv.locations[j];

      if (loc.root.empty())
        loc.root = srv.root; // inherit from server
      if (loc.index_files.empty())
        loc.index_files = srv.index_files;
      if (loc.methods.empty())
        loc.methods = srv.methods;
    }
  }
}

void validateCompatibility(const Config &cfg) {
  const std::vector<ServerConfig> &servers = cfg.getServers();
  for (size_t i = 0; i < servers.size(); ++i) {
    const ServerConfig &srv = servers[i];
    for (size_t j = 0; j < srv.locations.size(); ++j) {
      const LocationConfig &loc = srv.locations[j];

      // Example: POST on a location that doesn't look like an upload endpoint.
      // (No CGI feature in our minimal structs yet; add a check later if/when
      // CGI exists.)
      if (loc.methods.count("POST") && loc.redirect.status == 0 &&
          loc.root.find("upload") == std::string::npos) {
        std::cerr
            << "Warning: Location " << loc.path
            << " allows POST but is not obviously an upload route (no 'upload' "
               "in root and no redirect). "
               "Consider restricting methods or implementing POST handling.\n";
      }

      // Example: DELETE on static root
      if (loc.methods.count("DELETE") && loc.redirect.status != 0) {
        std::cerr << "Warning: Location " << loc.path
                  << " defines DELETE but has a redirect\n";
      }
    }
  }
}
