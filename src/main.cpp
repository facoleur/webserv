// main.cpp

#include "ConfigFile.hpp"
#include "ConfigParser.hpp"
#include "Server.hpp"

int main(int argc, char const* argv[]) {
    const char* path = (argc > 1) ? argv[1] : "config/default.conf";
    // Optional: validate the path
    // ConfigFile::checkFile etc. kept minimal here
    ConfigParser parser;
    Config       cfg = parser.parseFile(path);
    applyDefaults(cfg);
    validateCompatibility(cfg); // try catch => if catch, return

    Server serv(cfg);
    serv.run();
    return 0;
}
