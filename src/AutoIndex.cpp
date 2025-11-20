#include "AutoIndex.hpp"

std::string& replaceVariables(std::string& html, const std::string& variable, const std::string& value) {
    if (html.find("{{" + variable + "}}") == std::string::npos)
        return html;

    size_t pos = html.find("{{" + variable + "}}");
    html.replace(pos, variable.size() + 4, value);

    replaceVariables(html, variable, value);

    return html;
}

std::string AutoIndex::fillTemplate(const std::string& dir, const std::vector<AutoIndexItem>& items) {
    std::ifstream templateFile("www/templates/autoindex.html");
    std::string   htmlTemplate = readFile(templateFile);
    replaceVariables(htmlTemplate, "path", dir);

    std::ifstream linkFile("www/templates/link.html");

    std::string links;

    std::string htmlLink = readFile(linkFile);
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
