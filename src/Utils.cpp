// Utils.cpp

#include "Utils.hpp"
#include <dirent.h>
#include <fstream>
#include <poll.h>
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

std::ostream& operator<<(std::ostream& os, struct pollfd pfd) {
    os << "fd: " << pfd.fd << std::endl;
    os << "events: " << pfd.events << std::endl;
    os << "revents: " << pfd.revents << std::endl;
    return os;
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
