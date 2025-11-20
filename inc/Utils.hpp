#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <vector>
#include "ConfigParser.hpp"

std::string              toString(int n);
void                     replace(std::string& str, const std::string& from, const std::string& to);
std::vector<std::string> split(const std::string& s, char delimiter);
bool                     startsWith(const std::string& str, const std::string& prefix);
bool                     endsWith(const std::string& str, const std::string& prefix);
std::string              join(const std::vector<std::string>& parts, char delim);
bool                     isDirectory(const std::string& path);
std::string				methodToString(requestMethod method);
std::string				trimString(const std::string& value);
std::string				toLower(const std::string& str);
bool					isSubPath(const std::string& root, const std::string& candidate);
requestMethod			toMethod(const std::string& s);
