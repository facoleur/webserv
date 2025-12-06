#include "Utils.hpp"
#include <fstream>
#include <sys/types.h>

#include "AutoIndex.hpp"

std::string AutoIndex::fillTemplate(const std::string& dir, const std::vector<AutoIndexItem>& items) {
    std::ifstream templateFile("www/templates/autoindex.html");
    std::string   htmlTemplate = readFile(templateFile);
    replaceVariables(htmlTemplate, "path", dir);

    std::string links;

    std::ifstream linkFile("www/templates/link.html");
    std::string   htmlLink = readFile(linkFile);
    for (uint i = 0; i < items.size(); i++) {
        std::string tmp = htmlLink;
        replaceVariables(tmp, "href", items[i].path);
        replaceVariables(tmp, "label", items[i].name);
        replaceVariables(tmp, "size", items[i].type ? toString(items[i].size) : "");
        replaceVariables(tmp, "lastmod", items[i].lastmod);
        links += tmp;
    }

    replaceVariables(htmlTemplate, "items", links);

    return htmlTemplate;
}

struct IsSpace {
    bool operator()(char c) const {
        return std::isspace(static_cast<unsigned char>(c));
    }
};

AutoIndex::AutoIndex() {
}

AutoIndex::~AutoIndex() {
}
