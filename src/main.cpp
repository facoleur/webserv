// main.cpp

#include <stdlib.h>

#include "ConfigFile.hpp"
#include "ConfigParser.hpp"
#include "Logger.hpp"
#include "Server.hpp"

int main(int argc, char const* argv[]) {
    const char* path = (argc > 1) ? argv[1] : "config/default.conf";

    // Validate config path early
    std::string error;
    if (!ConfigFile::validateConfigPath(path, error)) {
        LOG_ERROR(error);
        return 1;
    }
    ConfigParser parser;
    Config       cfg;
    try {
        cfg = parser.parseFile(path);
    } catch (ParseError& pe) {
        LOG_ERROR(pe.what());
        return -1;
    }
    applyDefaults(cfg);
    try {
        validateCompatibility(cfg);
    } catch (std::runtime_error& re) {
        LOG_ERROR(re.what());
        LOG_ERROR("Aborting...");
        return 0;
    }

    Server serv(cfg);
    LOG_INFO("Starting server...")
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
