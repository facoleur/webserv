#include "Enums.hpp"
#include "Webserv.hpp"
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <vector>

class ConfigParser;

std::string&             replaceVariables(std::string&, const std::string&, const std::string&);
std::string              methodToString(requestMethod);
std::string              trimString(const std::string&);
std::string              toLower(const std::string&);
bool                     isSubPath(const std::string&, const std::string&);
requestMethod            toMethod(const std::string&);
std::string              toString(long long);
size_t                   toSizet(const std::string&);
std::string              tolower(const std::string&);
void                     replace(std::string&, const std::string&, const std::string&);
std::vector<std::string> split(const std::string&, char);
bool                     startsWith(const std::string&, const std::string&);
bool                     endsWith(const std::string&, const std::string&);
std::string              join(const std::vector<std::string>&, char);
bool                     isDirectory(const std::string&);
std::string              readFile(const std::ifstream&);
std::string              getParentDir(const std::string&);
bool                     isSpace(int);
bool                     isAcceptedHeader(std::string&);

// OutstreamUtils.cpp
std::ostream& operator<<(std::ostream&, const struct pollfd);
std::ostream& operator<<(std::ostream&, const headersMap&);
std::ostream& operator<<(std::ostream&, requestHeaders);
std::ostream& operator<<(std::ostream&, requestMethod);
std::ostream& operator<<(std::ostream&, requestValidity);
std::ostream& operator<<(std::ostream&, statusCode);
