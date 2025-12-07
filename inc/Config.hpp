#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "Enums.hpp"

struct Redirect {
    int         status; // 0 = pas de redir
    std::string target; // comme "/picture.img" ou "http://..."
    Redirect() : status(0) {
    }
};

struct LocationConfig {
    std::string              path; // comme "/picture.img" ou "http://..."
    std::string              root;
    std::vector<std::string> indexFiles;
    std::set<requestMethod>  methods; // {"GET", "POST", "DELETE"}
    Redirect                 redirect;
    // New features
    bool                               autoindex;         // inherited from server if unspecified
    size_t                             clientMaxBodySize; // 0 means unspecified -> inherit
    std::map<std::string, std::string> cgiMap;            // ext -> interpreter
    bool                               uploadEnable;      // default false
    std::string                        uploadStore;       // empty if not set
    std::map<int, std::string>         errorPages;        // code -> path

    // Internal flag to distinguish explicit autoindex vs inherit
    bool autoindexSet;
    LocationConfig()
        : path(), root(), indexFiles(), methods(), redirect(), autoindex(false), clientMaxBodySize(0), cgiMap(),
          uploadEnable(false), uploadStore(), autoindexSet(false) {
    }
};

struct ServerConfig {
    std::string              host; // comme "127.0.0.1"
    std::string              serverName;
    std::string              root;
    std::vector<std::string> indexFiles;
    std::set<requestMethod>  methods;
    Redirect                 redirect;
    // New features
    std::vector<int>                   listenPorts;
    std::map<int, std::string>         errorPages; // code -> path
    bool                               autoindex;
    size_t                             clientMaxBodySize; // 0 means apply default
    std::map<std::string, std::string> cgiMap;            // ext -> interpreter
    std::vector<LocationConfig>        locations;
    ServerConfig()
        : host(), root(), indexFiles(), methods(), redirect(), listenPorts(), errorPages(), autoindex(false),
          clientMaxBodySize(0), cgiMap(), locations() {
    }

    bool matchServerName(const std::string& hostHeader) const;
};

class Config {
  public:
    // Mutations
    void clearServers() {
        servers.clear();
    }
    void addServer(const ServerConfig& s) {
        servers.push_back(s);
    }

    // Queries
    size_t serverCount() const {
        return servers.size();
    }
    const std::vector<ServerConfig>& getServers() const {
        return servers;
    }
    std::vector<ServerConfig>& getServers() {
        return servers;
    }

    operator bool() const {
        return !servers.empty();
    }

  private:
    std::vector<ServerConfig> servers;
};

void applyDefaults(Config& cfg);
void validateCompatibility(const Config& cfg);
