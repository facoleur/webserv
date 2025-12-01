// Utils.cpp

#include "Utils.hpp"
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/stat.h>

std::string toString(long long n) {

    std::stringstream ss;
    ss << n;
    return ss.str();
}
size_t toSizet(const std::string& s) {
    size_t n = 0;
    for (size_t i = 0; i < s.size(); i++)
        n = n * 10 + (s[i] - '0');
    return n;
}

std::string tolower(const std::string& s) {
    std::string out = s;
    for (size_t i = 0; i < out.size(); i++)
        out[i] = std::tolower(static_cast<unsigned char>(out[i]));
    return out;
}

void replace(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty())
        return;
    std::string::size_type start = 0;
    while ((start = str.find(from, start)) != std::string::npos) {
        str.replace(start, from.length(), to);
        start += to.length();
    }
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream        ss(s);
    std::string              item;

    while (std::getline(ss, item, delimiter)) {
        tokens.push_back(item);
    }

    return tokens;
}

bool startsWith(const std::string& str, const std::string& prefix) {
    if (prefix.size() > str.size())
        return false;
    return std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool endsWith(const std::string& str, const std::string& prefix) {
    if (prefix.size() > str.size())
        return false;
    return std::equal(prefix.rbegin(), prefix.rend(), str.rbegin());
}

std::string join(const std::vector<std::string>& parts, char delim) {
    std::string out;
    for (std::vector<std::string>::size_type i = 0; i < parts.size(); ++i) {
        if (i)
            out += delim;
        out += parts[i];
    }
    return out;
}

bool isDirectory(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false;
    return S_ISDIR(info.st_mode);
}

std::string readFile(const std::ifstream& file) {
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

std::string getParentDir(const std::string& path) {
    if (path.empty())
        return "";

    std::string trimmed = path;
    while (!trimmed.empty() && trimmed[trimmed.size() - 1] == '/')
        trimmed.erase(trimmed.size() - 1);

    std::string::size_type pos = trimmed.rfind('/');
    if (pos == std::string::npos)
        return "";

    if (pos == 0)
        return "/";

    return trimmed.substr(0, pos);
}

bool isSpace(int i) {
    return (std::isspace(i));
}

std::string& replaceVariables(std::string& html, const std::string& variable, const std::string& value) {
    if (html.find("{{" + variable + "}}") == std::string::npos)
        return html;

    size_t pos = html.find("{{" + variable + "}}");
    html.replace(pos, variable.size() + 4, value);

    replaceVariables(html, variable, value);

    return html;
}

std::string methodToString(requestMethod method) {
    switch (method) {
        case GET:
            return "GET";
        case POST:
            return "POST";
        case DELETE:
            return "DELETE";
        default:
            return "UNKNOWN";
    }
}

// trims whitespace at the beginning and end of a std::string
std::string trimString(const std::string& str) {
    std::string::const_iterator begin = str.begin();
    std::string::const_iterator end   = str.end();

    // move begin forward while it points to whitespace
    while (begin != end && isSpace(static_cast<unsigned char>(*begin)))
        ++begin;

    // move end backward while it points to whitespace
    if (begin != end) {
        do {
            --end;
        } while (end != begin && isSpace(static_cast<unsigned char>(*end)));
        ++end; // move back to one-past-last non-space
    }
    return std::string(begin, end);
}

std::string toLower(const std::string& str) {
    std::string lowered = str;
    for (size_t i = 0; i < lowered.size(); ++i) {
        lowered[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[i])));
    }
    return lowered;
}

unsigned char toLowerChar(unsigned char c) {
    return static_cast<unsigned char>(std::tolower(c));
}

bool isSubPath(const std::string& root, const std::string& candidate) {
    if (root.empty())
        return true;
    if (candidate.size() < root.size())
        return false;
    if (candidate.compare(0, root.size(), root) != 0)
        return false;
    if (candidate.size() == root.size())
        return true;
    char last = root[root.size() - 1];
    if (last == '/')
        return true;
    return candidate[root.size()] == '/';
}

// Map a method string to the project's enum
enum requestMethod toMethod(const std::string& s) {
    if (s == "GET")
        return GET;
    if (s == "POST")
        return POST;
    if (s == "DELETE")
        return DELETE;
    return UNKNOWN;
}
