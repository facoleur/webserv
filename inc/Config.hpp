#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

#include "Request.hpp" // for enum requestMethod

struct Redirect {
  int status;         // 0 = pas de redir
  std::string target; // comme "/picture.img" ou "http://..."
  Redirect() : status(0) {}
};

/* Subject requires a directory listing toggle. Keep autoindex in model/parser;
default off; only use it at runtime when serving dirs. If you truly
won’t implement listings now, still parse/store it for spec compliance.
 */

struct LocationConfig {
  std::string path; // comme "/picture.img" ou "http://..."
  std::string root;
  std::vector<std::string> index_files;
  std::set<enum requestMethod> methods; // {"GET", "POST", "DELETE"}
  Redirect redirect;
  // New features
  bool autoindex;              // inherited from server if unspecified
  size_t client_max_body_size; // 0 means unspecified -> inherit
  std::map<std::string, std::string> cgi_map; // ext -> interpreter
  bool upload_enable;                         // default false
  std::string upload_store;                   // empty if not set
  // Internal flag to distinguish explicit autoindex vs inherit
  bool autoindex_set;
  LocationConfig()
      : path(), root(), index_files(), methods(), redirect(), autoindex(false),
        client_max_body_size(0), cgi_map(), upload_enable(false),
        upload_store(), autoindex_set(false) {}
};

struct ServerConfig {
  std::string host; // comme "127.0.0.1"
  std::string root;
  std::vector<std::string> index_files;
  std::set<enum requestMethod> methods;
  Redirect redirect;
  // New features
  std::vector<int> listen_ports;
  std::map<int, std::string> error_pages; // code -> path
  bool autoindex;
  size_t client_max_body_size;                // 0 means apply default
  std::map<std::string, std::string> cgi_map; // ext -> interpreter
  std::vector<LocationConfig> locations;
  ServerConfig()
      : host(), root(), index_files(), methods(), redirect(), listen_ports(),
        error_pages(), autoindex(false), client_max_body_size(0), cgi_map(),
        locations() {}
};

class Config {
public:
  // Mutations
  void clearServers() { servers.clear(); }
  void addServer(const ServerConfig &s) { servers.push_back(s); }

  // Queries
  size_t serverCount() const { return servers.size(); }
  const std::vector<ServerConfig> &getServers() const { return servers; }
  std::vector<ServerConfig> &getServers() { return servers; }

private:
  std::vector<ServerConfig> servers;
};

void applyDefaults(Config &cfg);
void validateCompatibility(const Config &cfg);

#endif
