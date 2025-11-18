// Utils.cpp

#include "Utils.hpp"

std::string toString(long long n) {

    std::stringstream ss;
    ss << n;
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, struct pollfd pfd) {
    os << "fd: " << pfd.fd << std::endl;
    os << "events: " << pfd.events << std::endl;
    os << "revents: " << pfd.revents << std::endl;
    return os;

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
