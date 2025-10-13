#include "Parser.hpp"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

ConfigParser::ConfigParser() : i_(0) {}

static bool isPunct(char c) { return c == '{' || c == '}' || c == ';'; }

static bool isSpace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' ||
         c == '\f';
}

std::vector<ConfigParser::Token>
ConfigParser::tokenize(const std::string &text) {}

bool ConfigParser::isMethod(const std::string &m) {
  return m == "GET" || m == "POST" || m == "DELETE";
}

int ConfigParser::toInt(const std::string &s) {
  // minimal, ne check pas overflow
  return std::atoi(s.c_str());
}

Config ConfigParser::parseFile(const std::string &path) {}

Config ConfigParser::parseString(const std::string &text) {}

void ConfigParser::parseConfig() {}

void ConfigParser::parseServer() {}
