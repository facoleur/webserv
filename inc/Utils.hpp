#include "Server.hpp"
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <vector>

std::ostream& operator<<(std::ostream& os, struct pollfd pfd);
std::string              toString(long long n);
void                     replace(std::string& str, const std::string& from, const std::string& to);
std::vector<std::string> split(const std::string& s, char delimiter);
bool                     startsWith(const std::string& str, const std::string& prefix);
bool                     endsWith(const std::string& str, const std::string& prefix);
std::string              join(const std::vector<std::string>& parts, char delim);
bool                     isDirectory(const std::string& path);
