#include <dirent.h>
#include <string>
#include <vector>

enum autoIndexType { T_DIR, T_FILE };

struct AutoIndexItem {
    std::string        path;
    std::string        name;
    size_t             size;
    enum autoIndexType type;
    std::string        lastmod;

    AutoIndexItem(const std::string& path, const std::string& name, size_t size, enum autoIndexType type,
                  const std::string& lastmod)
        : path(path), name(name), size(size), type(type), lastmod(lastmod) {
    }
};

class AutoIndex {
  public:
    AutoIndex();
    ~AutoIndex();

    static std::string fillTemplate(const std::string& dir, const std::vector<AutoIndexItem>& items);
};
