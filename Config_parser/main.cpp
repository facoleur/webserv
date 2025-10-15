#include "Parser.hpp"
#include <iostream>

int main(int argc, char **argv) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: " << argv[0] << " <config.conf>\n";
      return 1;
    }
    ConfigParser p;
    Config cfg = p.parseFile(argv[1]);
    std::cout << "OK. servers=" << cfg.servers.size() << "\n";
    for (size_t i = 0; i < cfg.servers.size(); ++i) {
      const ServerConfig &s = cfg.servers[i];
      std::cout << "server[" << i << "] host=" << s.host << " root=" << s.root
                << " index="
                << (s.index_files.empty() ? "(none)" : s.index_files[0])
                << "\n";
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