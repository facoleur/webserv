// main.cpp

#include <stdlib.h>

#include "ConfigFile.hpp"
#include "ConfigParser.hpp"
#include "Server.hpp"
#include <cstdlib>

int main(int argc, char const* argv[]) {
#ifndef DEBUG_MODE
    system("clear");
#endif
    const char* path = (argc > 1) ? argv[1] : "config/servername.conf";
    // Validate config path early
    std::string err;
    if (!ConfigFile::validateConfigPath(path, err)) {
        std::cout << "printing err here" << std::endl;
        std::cerr << err << "\n";
        return 1;
    }
    ConfigParser parser;
    Config       cfg;
    try {
        cfg = parser.parseFile(path);
    } catch (ParseError& pe) {
        std::cerr << pe.what() << std::endl << "aborting" << std::endl;
        return -1;
    }
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
