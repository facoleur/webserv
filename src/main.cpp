// main.cpp

#include "ConfigFile.hpp"
#include "ConfigParser.hpp"
#include "Server.hpp"
#include "Webserv.hpp"
#include <unistd.h>

int main(int argc, char const* argv[]) {
    const char* path = (argc > 1) ? argv[1] : "config/default.conf";
    // Validate config path early
    std::string err;
    if (!ConfigFile::validateConfigPath(path, err)) {
        std::cerr << err << "\n";
        return 1;
    }
    ConfigParser parser;
    Config       cfg = parser.parseFile(path);
    applyDefaults(cfg);
    validateCompatibility(cfg); // try catch => if catch, return

    Server serv(cfg);
    serv.run();
    return 0;
}
