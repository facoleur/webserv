// main.cpp

#include <csignal>
#include <stdlib.h>

#include "ConfigFile.hpp"
#include "ConfigParser.hpp"
#include "Logger.hpp"
#include "Server.hpp"

static Server* gServer = 0;

void handle_sigint(int signum) {
    (void)signum;
    gServer->clean();
    std::exit(0);
}

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

    std::signal(SIGINT, handle_sigint);
    Server server(cfg);
    gServer = &server;

    LOG_INFO("Starting server...")
    server.run();

    return 0;
}
