#pragma once

#include "Config.hpp"
#include "Enums.hpp"
#include "Server.hpp"
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <vector>

class ConfigParser;

std::string              getCurrentDatetime(const std::string& format = "%a, %d %h %G %H:%M:%S %Z");
std::string&             replaceVariables(std::string&, const std::string&, const std::string&);
std::string              methodToString(requestMethod);
std::string              trimString(const std::string&);
bool                     isSubPath(const std::string&, const std::string&);
requestMethod            toMethod(const std::string&);
std::string              toString(long long);
size_t                   toSizet(const std::string&);
std::string              toLower(const std::string&);
unsigned char            toLowerChar(unsigned char);
void                     replace(std::string&, const std::string&, const std::string&);
std::vector<std::string> split(const std::string&, char);
bool                     startsWith(const std::string&, const std::string&);
bool                     endsWith(const std::string&, const std::string&);
std::string              join(const std::vector<std::string>&, char);
bool                     isDirectory(const std::string&);
std::string              readFile(const std::ifstream&);
std::string              getParent(const std::string&);
bool                     isSpace(int);
bool                     isAcceptedHeader(std::string&);
void                     removeDoubleSlash(std::string&);
void                     initHeaderStringToEnumMap(std::map<std::string, requestHeaders>&);
const ServerConfig&      getServerConfig(const ClientContext& context, const Config& config, const std::string& host);

// OutstreamUtils.cpp
std::ostream& operator<<(std::ostream&, const struct pollfd);
std::ostream& operator<<(std::ostream&, const headersMap&);
std::ostream& operator<<(std::ostream&, requestHeaders);
std::ostream& operator<<(std::ostream&, requestMethod);
std::ostream& operator<<(std::ostream&, statusCode);
