// main.cpp

#include "ConfigFile.hpp"
#include "ConfigParser.hpp"
#include "Server.hpp"
#include <iostream>
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

// #include "Config.hpp"
// #include "Request.hpp"
// #include "RequestRouter.hpp"
// #include "Response.hpp"

// int main() {
//     RequestRouter router;

//     std::vector<std::string> paths;

//     paths.push_back("www/dir");

//     router.makeAutoindexResponse(paths[0]);

//     return 0;
// }
