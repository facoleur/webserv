#include "Webserv.hpp"
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <vector>

class ConfigParser;

std::ostream&            operator<<(std::ostream& os, struct pollfd pfd);
std::string              toString(long long n);
size_t                   toSizet(const std::string& s);
std::string              tolower(const std::string& s);
void                     replace(std::string& str, const std::string& from, const std::string& to);
std::vector<std::string> split(const std::string& s, char delimiter);
bool                     startsWith(const std::string& str, const std::string& prefix);
bool                     endsWith(const std::string& str, const std::string& prefix);
std::string              join(const std::vector<std::string>& parts, char delim);
bool                     isDirectory(const std::string& path);
std::string              readFile(const std::ifstream&);
std::string              getParentDir(const std::string& path);
bool                     isSpace(int i);
bool                     isAcceptedHeader(std::string&);
std::string&             replaceVariables(std::string&, const std::string&, const std::string& value);
std::string              methodToString(enum requestMethod method);
std::string              trimString(const std::string& value);
std::string              toLower(const std::string& str);
bool                     isSubPath(const std::string& root, const std::string& candidate);
enum requestMethod       toMethod(const std::string& s);
