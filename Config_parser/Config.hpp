#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

struct Redirect {
  int status;         // 0 = pas de redir
  std::string target; // comme "/picture.img" ou "http://..."
  Redirect() : status(0) {}
};

struct LocationConfig {
  std::string path; // comme "/picture.img" ou "http://..."
  std::string root;
  std::vector<std::string> index_files;
  std::set<std::string> methods; // {"GET", "POST", "DELETE"}
  Redirect redirect;
};

struct ServerConfig {
  std::string host; // comme "127.0.0.1"
  std::string root;
  std::vector<std::string> index_files;
  std::set<std::string> methods;
  Redirect redirect;
  std::vector<LocationConfig> locations;
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