#pragma once

#include <string>
#include <vector>

enum PathType { PATH_ERROR = -1, PATH_FILE = 1, PATH_DIR = 2, PATH_OTHER = 3 };

class ConfigFile {
  public:
    ConfigFile();

    static PathType getTypePath(const std::string& path);
    static int      checkFile(const std::string& path, int mode);
    static int      isFileExistAndReadable(const std::string& path, const std::string& index);
    // Validate config path is a readable regular file; return false with err msg
    static bool validateConfigPath(const std::string& path, std::string& err);

    std::string        readFile(const std::string& path);
    const std::string& getPath() const;
    int                getSize() const;

  private:
    std::string path_;
    int         size_;
};

struct ServerBlock {
    std::vector<int> ports;
    std::string      host;
    std::string      serverName;

    bool operator==(const ServerBlock& other) const {
        return ports == other.ports && host == other.host && serverName == other.serverName;
    }

    ServerBlock(std::vector<int> ports, const std::string& host, const std::string& serverName)
        : ports(ports), host(host), serverName(serverName) {
    }
};
