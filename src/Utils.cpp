// Utils.cpp

#include "Utils.hpp"


std::string toString(int n) {

    std::stringstream ss;
    ss << n;
    return ss.str();
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

std::string trimString(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && (value[start] == ' ' || value[start] == '\t' || value[start] == '\r'))
        ++start;
    size_t end = value.size();
    while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r'))
        --end;
    return value.substr(start, end - start);
}

std::string toLower(const std::string& str) {
    std::string lowered = str;
    for (size_t i = 0; i < lowered.size(); ++i) {
        lowered[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[i])));
    }
    return lowered;
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
requestMethod toMethod(const std::string& s) {
    if (s == "GET")
        return GET;
    if (s == "POST")
        return POST;
    if (s == "DELETE")
        return DELETE;
    return UNKNOWN;
}