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
ConfigParser::tokenize(const std::string &text) {
  std::vector<Token> out;
  size_t n = text.size();
  size_t i = 0;
  size_t line = 1, col = 1;

  while (i < n) {
    char c = text[i];

    // Skip whitespace
    if (isSpace(c)) {
      if (c == '\n') {
        line++;
        col = 1;
      } else {
        col++;
      }
      i++;
      continue;
    }

    // Comments: '#' to end-of-line
    if (c == '#') {
      while (i < n && text[i] != '\n') {
        i++;
        col++;
      }
      continue;
    }

    // C++-style comment //
    if (c == '/' && i + 1 < n && text[i + 1] == '/') {
      while (i < n && text[i] != '\n') {
        i++;
        col++;
      }
      continue;
    }

    // Punctuation tokens
    if (isPunct(c)) {
      Token t;
      t.s = std::string(1, c);
      t.line = line;
      t.col = col;
      out.push_back(t);
      i++;
      col++;
      continue;
    }

    // Word token: read until whitespace or punctuation
    size_t start_i = i;
    size_t start_col = col;
    while (i < n && !isSpace(text[i]) && !isPunct(text[i]) && text[i] != '#') {
      // stop before '//' comment start
      if (text[i] == '/' && i + 1 < n && text[i + 1] == '/')
        break;
      i++;
      col++;
    }
    Token t;
    t.s.assign(text.begin() + start_i, text.begin() + i);
    t.line = line;
    t.col = start_col;
    if (!t.s.empty())
      out.push_back(t);
  }
  return out;
}

bool ConfigParser::isMethod(const std::string &m) {
  return m == "GET" || m == "POST" || m == "DELETE";
}

int ConfigParser::toInt(const std::string &s) {
  // minimal, no overflow checks
  return std::atoi(s.c_str());
}

Config ConfigParser::parseFile(const std::string &path) {
  std::ifstream in(path.c_str());
  if (!in)
    throw ParseError("Cannot open config file: " + path);
  std::ostringstream oss;
  oss << in.rdbuf();
  return parseString(oss.str());
}

Config ConfigParser::parseString(const std::string &text) {
  cfg_.servers.clear();
  toks_ = tokenize(text);
  i_ = 0;
  parseConfig();
  return cfg_;
}

void ConfigParser::parseConfig() {
  while (!eof()) {
    expect("server", "expected 'server'");
    expect("{", "expected '{' after server");
    parseServer();
  }
}

void ConfigParser::parseServer() {
  ServerConfig srv;
  while (!accept("}")) {
    if (eof())
      throw ParseError("unexpected EOF inside server block");

    if (accept("location")) {
      parseLocation(srv);
      continue;
    }

    if (accept("host")) {
      Token ip = next();
      if (ip.s == ";" || ip.s == "{" || ip.s == "}" || ip.s.empty())
        throw ParseError("invalid host value at line " + std::string(1, '\0'));
      srv.host = ip.s;
      expect(";", "missing ';' after host");
      continue;
    }
    if (accept("root")) {
      Token p = next();
      if (p.s == ";" || p.s == "{" || p.s == "}" || p.s.empty())
        throw ParseError("invalid root value (line " + std::string(1, '\0') +
                         ")");
      srv.root = p.s;
      expect(";", "missing ';' after root");
      continue;
    }
    if (accept("index")) {
      // at least one filename, until ';'
      while (!accept(";")) {
        if (eof())
          throw ParseError("unexpected EOF in index directive");
        Token f = next();
        if (f.s == "{" || f.s == "}" || f.s == ";")
          throw ParseError("invalid token in index directive at line " +
                           std::string(1, '\0'));
        srv.index_files.push_back(f.s);
      }
      continue;
    }
    if (accept("methods")) {
      bool has = false;
      while (!accept(";")) {
        if (eof())
          throw ParseError("unexpected EOF in methods directive");
        Token m = next();
        if (!isMethod(m.s))
          throw ParseError("invalid method '" + m.s +
                           "' (allowed: GET POST DELETE)");
        srv.methods.insert(m.s);
        has = true;
      }
      if (!has)
        throw ParseError("methods directive requires at least one method");
      continue;
    }
    if (accept("return")) {
      Token st = next();
      Token tgt = next();
      int code = toInt(st.s);
      if (code <= 0 || tgt.s.empty() || tgt.s == "{" || tgt.s == "}" ||
          tgt.s == ";")
        throw ParseError("invalid return directive");
      srv.redirect.status = code;
      srv.redirect.target = tgt.s;
      expect(";", "missing ';' after return");
      continue;
    }

    // Unknown directive in server
    Token t = next();
    std::ostringstream oss;
    oss << "unknown directive '" << t.s << "' in server (line " << t.line
        << ", col " << t.col << ")";
    throw ParseError(oss.str());
  }
  cfg_.servers.push_back(srv);
}

void ConfigParser::parseLocation(ServerConfig &srv) {
  LocationConfig loc;
  Token path = next();
  if (path.s.empty() || path.s == "{" || path.s == "}" || path.s == ";")
    throw ParseError("invalid location path");
  loc.path = path.s;
  expect("{", "expected '{' after location path");

  while (!accept("}")) {
    if (eof())
      throw ParseError("unexpected EOF inside location block");

    if (accept("root")) {
      Token p = next();
      if (p.s == ";" || p.s == "{" || p.s == "}" || p.s.empty())
        throw ParseError("invalid root value in location");
      loc.root = p.s;
      expect(";", "missing ';' after root");
      continue;
    }
    if (accept("index")) {
      while (!accept(";")) {
        if (eof())
          throw ParseError("unexpected EOF in index directive");
        Token f = next();
        if (f.s == "{" || f.s == "}" || f.s == ";")
          throw ParseError("invalid token in index directive (location)");
        loc.index_files.push_back(f.s);
      }
      continue;
    }
    if (accept("methods")) {
      bool has = false;
      while (!accept(";")) {
        if (eof())
          throw ParseError("unexpected EOF in methods directive");
        Token m = next();
        if (!isMethod(m.s))
          throw ParseError("invalid method '" + m.s + "' in location");
        loc.methods.insert(m.s);
        has = true;
      }
      if (!has)
        throw ParseError(
            "methods directive requires at least one method (location)");
      continue;
    }
    if (accept("return")) {
      Token st = next();
      Token tgt = next();
      int code = toInt(st.s);
      if (code <= 0 || tgt.s.empty() || tgt.s == "{" || tgt.s == "}" ||
          tgt.s == ";")
        throw ParseError("invalid return directive (location)");
      loc.redirect.status = code;
      loc.redirect.target = tgt.s;
      expect(";", "missing ';' after return");
      continue;
    }

    // No 'host' allowed here in our minimal grammar:
    Token t = next();
    std::ostringstream oss;
    oss << "unknown directive '" << t.s << "' in location (line " << t.line
        << ", col " << t.col << ")";
    throw ParseError(oss.str());
  }

  srv.locations.push_back(loc);
}

bool ConfigParser::accept(const std::string &kw) {
  if (!eof() && toks_[i_].s == kw) {
    i_++;
    return true;
  }
  return false;
}

void ConfigParser::expect(const std::string &kw, const char *err) {
  if (!accept(kw)) {
    std::ostringstream oss;
    oss << (err ? err : "expected token") << " (got '";
    if (!eof())
      oss << toks_[i_].s << "' at line " << toks_[i_].line << ", col "
          << toks_[i_].col << ")";
    else
      oss << "EOF')";
    throw ParseError(oss.str());
  }
}

ConfigParser::Token ConfigParser::next() {
  if (eof())
    throw ParseError("unexpected EOF");
  return toks_[i_++];
}

const ConfigParser::Token &ConfigParser::peek() const {
  if (eof())
    throw ParseError("unexpected EOF");
  return toks_[i_];
}

bool ConfigParser::eof() const { return i_ >= toks_.size(); }