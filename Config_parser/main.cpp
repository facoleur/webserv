#include "Config.hpp"
#include "ConfigFile.hpp"
#include "Parser.hpp"
#include <iostream>
#include <string>
#include <unistd.h>

int main(int argc, char **argv) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: " << argv[0] << " <config.conf>\n";
      return 1;
    }
    const std::string configPath(argv[1]);
    int type = ConfigFile::getTypePath(configPath);
    if (type == -1) {
      std::cerr << "Error: cannot access config file '" << configPath << "'\n";
      return 1;
    }
    if (type != 1) {
      std::cerr << "Error: '" << configPath << "' is not a regular file\n";
      return 1;
    }
    if (ConfigFile::checkFile(configPath, R_OK) != 0) {
      std::cerr << "Error: '" << configPath
                << "' is not readable (check permissions)\n";
      return 1;
    }

    ConfigParser p;
    Config cfg = p.parseFile(configPath);
    applyDefaults(cfg);
    const std::vector<ServerConfig> &servers = cfg.getServers();
    validateCompatibility(cfg);
    std::cout << "OK. servers=" << cfg.serverCount() << "\n";
    for (size_t i = 0; i < cfg.serverCount(); ++i) {
      const ServerConfig &s = servers[i];
      std::cout << "server[" << i << "] host=" << s.host << " root=" << s.root
                << " index=";
      if (s.index_files.empty()) {
        std::cout << "(none)";
      } else {
        for (size_t k = 0; k < s.index_files.size(); ++k) {
          std::cout << s.index_files[k];
          if (k + 1 < s.index_files.size())
            std::cout << ", ";
        }
      }
      std::cout << "\n";
      for (size_t j = 0; j < s.locations.size(); ++j) {
        const LocationConfig &l = s.locations[j];
        std::cout << "  location " << l.path << " root=" << l.root
                  << " idxCount=" << l.index_files.size() << "\n";
      }
    }
  } catch (const ParseError &e) {
    std::cerr << "Config error: " << e.what() << "\n";
    return 2;
  }
}
