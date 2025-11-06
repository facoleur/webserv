#include "ConfigParser.hpp"
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

static bool isIPv4(const std::string &s) {
  // Accept only dotted-quad a.b.c.d with each part 0..255 and only digits
  int parts = 0;
  size_t i = 0, n = s.size();
  while (i < n) {
    if (parts == 4)
      return false; // too many parts
    if (!std::isdigit(static_cast<unsigned char>(s[i])))
      return false;

    int val = 0;
    int digits = 0;

    while (i < n && std::isdigit(static_cast<unsigned char>(s[i]))) {
      val = val * 10 + (s[i] - '0');
      if (val > 255)
        return false;
      ++i;
      ++digits;
    }
    if (digits == 0)
      return false;

    ++parts;

    if (parts < 4) {
      if (i >= n || s[i] != '.')
        return false; // need dot between parts
      ++i;            // skip '.'
      if (i >= n)
        return false; // must have digits after dot
    }
  }
  return parts == 4;
}

// Return true if token is a directive keyword (not valid as index filename)
static bool isDirectiveKeyword(const std::string &s) {
  return s == "root" || s == "methods" || s == "return" || s == "location" ||
         s == "host" || s == "listen" || s == "error_page" ||
         s == "autoindex" || s == "client_max_body_size" || s == "cgi" ||
         s == "upload_enable" || s == "upload_store";
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

// Map a method string to the project's enum
static requestMethod toMethod(const std::string &s) {
  if (s == "GET") return GET;
  if (s == "POST") return POST;
  if (s == "DELETE") return DELETE;
  return UNKNOWN;
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
  cfg_.clearServers();
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
      if (ip.s == ";" || ip.s == "{" || ip.s == "}" || ip.s.empty()) {
        std::ostringstream msg;
        msg << "invalid host value '" << ip.s << "' at line " << ip.line
            << ", col " << ip.col;
        throw ParseError(msg.str());
      }
      if (!isIPv4(ip.s)) {
        std::ostringstream msg;
        msg << "invalid IPv4 address '" << ip.s << "' at line " << ip.line
            << ", col " << ip.col;
        throw ParseError(msg.str());
      }
      srv.host = ip.s;
      expect(";", "missing ';' after host");
      continue;
    }

    // Server-only + common directives
    if (parseDirectiveListen(srv) || parseDirectiveErrorPage(srv) ||
        parseDirectiveAutoIndex(srv) ||
        parseDirectiveClientMaxBodySize(srv) || parseDirectiveCgi(srv) ||
        parseDirectiveRoot(srv) || parseDirectiveIndex(srv) ||
        parseDirectiveMethods(srv) || parseDirectiveReturn(srv)) {
      continue;
    }

    // Unknown directive in server
    Token t = next();
    std::ostringstream oss;
    oss << "unknown directive '" << t.s << "' in server (line " << t.line
        << ", col " << t.col << ")";
    throw ParseError(oss.str());
  }
  cfg_.addServer(srv);
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

    // Shared directives + location-only
    if (parseDirectiveAutoIndex(loc) ||
        parseDirectiveClientMaxBodySize(loc) || parseDirectiveCgi(loc) ||
        parseDirectiveUploadEnable(loc) || parseDirectiveUploadStore(loc) ||
        parseDirectiveRoot(loc) || parseDirectiveIndex(loc) ||
        parseDirectiveMethods(loc) || parseDirectiveReturn(loc)) {
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

// -------- Reusable directive handlers (server + location) --------

bool ConfigParser::parseDirectiveListen(ServerConfig &srv) {
  if (!accept("listen"))
    return false;
  Token p = next();
  int port = toInt(p.s);
  if (port < 1 || port > 65535)
    throw ParseError("invalid listen port");
  srv.listen_ports.push_back(port);
  expect(";", "missing ';' after listen");
  return true;
}

bool ConfigParser::parseDirectiveErrorPage(ServerConfig &srv) {
  if (!accept("error_page"))
    return false;
  Token c = next();
  int code = toInt(c.s);
  Token path = next();
  if (code < 100 || code > 599 || path.s.empty() || path.s == ";" ||
      path.s == "{" || path.s == "}")
    throw ParseError("invalid error_page directive");
  srv.error_pages[code] = path.s;
  expect(";", "missing ';' after error_page");
  return true;
}

bool ConfigParser::parseDirectiveAutoIndex(ServerConfig &srv) {
  if (!accept("autoindex"))
    return false;
  Token v = next();
  if (v.s == "on")
    srv.autoindex = true;
  else if (v.s == "off")
    srv.autoindex = false;
  else
    throw ParseError("invalid autoindex value (use on|off)");
  expect(";", "missing ';' after autoindex");
  return true;
}

bool ConfigParser::parseDirectiveAutoIndex(LocationConfig &loc) {
  if (!accept("autoindex"))
    return false;
  Token v = next();
  if (v.s == "on")
    loc.autoindex = true;
  else if (v.s == "off")
    loc.autoindex = false;
  else
    throw ParseError("invalid autoindex value (use on|off)");
  loc.autoindex_set = true;
  expect(";", "missing ';' after autoindex");
  return true;
}

bool ConfigParser::parseDirectiveClientMaxBodySize(ServerConfig &srv) {
  if (!accept("client_max_body_size"))
    return false;
  Token v = next();
  int sz = toInt(v.s);
  if (sz <= 0)
    throw ParseError("invalid client_max_body_size (must be > 0)");
  srv.client_max_body_size = static_cast<size_t>(sz);
  expect(";", "missing ';' after client_max_body_size");
  return true;
}

bool ConfigParser::parseDirectiveClientMaxBodySize(LocationConfig &loc) {
  if (!accept("client_max_body_size"))
    return false;
  Token v = next();
  int sz = toInt(v.s);
  if (sz <= 0)
    throw ParseError("invalid client_max_body_size (must be > 0)");
  loc.client_max_body_size = static_cast<size_t>(sz);
  expect(";", "missing ';' after client_max_body_size");
  return true;
}

bool ConfigParser::parseDirectiveCgi(ServerConfig &srv) {
  if (!accept("cgi"))
    return false;
  Token ext = next();
  Token interp = next();
  if (ext.s.empty() || ext.s[0] != '.' || interp.s.empty() || interp.s == ";")
    throw ParseError("invalid cgi directive (use: cgi .ext /path/to/interpreter;)");
  srv.cgi_map[ext.s] = interp.s;
  expect(";", "missing ';' after cgi");
  return true;
}

bool ConfigParser::parseDirectiveCgi(LocationConfig &loc) {
  if (!accept("cgi"))
    return false;
  Token ext = next();
  Token interp = next();
  if (ext.s.empty() || ext.s[0] != '.' || interp.s.empty() || interp.s == ";")
    throw ParseError("invalid cgi directive (use: cgi .ext /path/to/interpreter;)");
  loc.cgi_map[ext.s] = interp.s;
  expect(";", "missing ';' after cgi");
  return true;
}

bool ConfigParser::parseDirectiveUploadEnable(LocationConfig &loc) {
  if (!accept("upload_enable"))
    return false;
  Token v = next();
  if (v.s == "on")
    loc.upload_enable = true;
  else if (v.s == "off")
    loc.upload_enable = false;
  else
    throw ParseError("invalid upload_enable value (use on|off)");
  expect(";", "missing ';' after upload_enable");
  return true;
}

bool ConfigParser::parseDirectiveUploadStore(LocationConfig &loc) {
  if (!accept("upload_store"))
    return false;
  Token p = next();
  if (p.s.empty() || p.s == ";" || p.s == "{" || p.s == "}")
    throw ParseError("invalid upload_store path");
  loc.upload_store = p.s;
  expect(";", "missing ';' after upload_store");
  return true;
}

bool ConfigParser::parseDirectiveRoot(ServerConfig &srv) {
  if (!accept("root"))
    return false;
  Token p = next();
  if (p.s == ";" || p.s == "{" || p.s == "}" || p.s.empty()) {
    std::ostringstream msg;
    msg << "invalid root value '" << p.s << "' at line " << p.line
        << ", col " << p.col;
    throw ParseError(msg.str());
  }
  srv.root = p.s;
  expect(";", "missing ';' after root");
  return true;
}

bool ConfigParser::parseDirectiveRoot(LocationConfig &loc) {
  if (!accept("root"))
    return false;
  Token p = next();
  if (p.s == ";" || p.s == "{" || p.s == "}" || p.s.empty())
    throw ParseError("invalid root value in location");
  loc.root = p.s;
  expect(";", "missing ';' after root");
  return true;
}

bool ConfigParser::parseDirectiveIndex(ServerConfig &srv) {
  if (!accept("index"))
    return false;
  while (!accept(";")) {
    if (eof())
      throw ParseError("unexpected EOF in index directive");
    Token f = next();
    // disallow punctuation and any known directive keywords inside index list
    if (f.s == "{" || f.s == "}" || f.s == ";" || isDirectiveKeyword(f.s)) {
      std::ostringstream msg;
      msg << "invalid token '" << f.s << "' in index directive at line "
          << f.line << ", col " << f.col;
      throw ParseError(msg.str());
    }
    srv.index_files.push_back(f.s);
  }
  return true;
}

bool ConfigParser::parseDirectiveIndex(LocationConfig &loc) {
  if (!accept("index"))
    return false;
  while (!accept(";")) {
    if (eof())
      throw ParseError("unexpected EOF in index directive");
    Token f = next();
    if (f.s == "{" || f.s == "}" || f.s == ";" || isDirectiveKeyword(f.s)) {
      std::ostringstream msg;
      msg << "invalid token '" << f.s << "' in index directive at line "
          << f.line << ", col " << f.col;
      throw ParseError(msg.str());
    }
    loc.index_files.push_back(f.s);
  }
  return true;
}

bool ConfigParser::parseDirectiveMethods(ServerConfig &srv) {
  if (!accept("methods"))
    return false;
  bool has = false;
  while (!accept(";")) {
    if (eof())
      throw ParseError("unexpected EOF in methods directive");
    Token m = next();
    requestMethod em = toMethod(m.s);
    if (em == UNKNOWN)
      throw ParseError("invalid method '" + m.s +
                       "' (allowed: GET POST DELETE)");
    srv.methods.insert(em);
    has = true;
  }
  if (!has)
    throw ParseError("methods directive requires at least one method");
  return true;
}

bool ConfigParser::parseDirectiveMethods(LocationConfig &loc) {
  if (!accept("methods"))
    return false;
  bool has = false;
  while (!accept(";")) {
    if (eof())
      throw ParseError("unexpected EOF in methods directive");
    Token m = next();
    requestMethod em = toMethod(m.s);
    if (em == UNKNOWN)
      throw ParseError("invalid method '" + m.s + "' in location");
    loc.methods.insert(em);
    has = true;
  }
  if (!has)
    throw ParseError("methods directive requires at least one method (location)");
  return true;
}

bool ConfigParser::parseDirectiveReturn(ServerConfig &srv) {
  if (!accept("return"))
    return false;
  Token st = next();
  Token tgt = next();
  int code = toInt(st.s);
  if (code <= 0 || tgt.s.empty() || tgt.s == "{" || tgt.s == "}" ||
      tgt.s == ";")
    throw ParseError("invalid return directive");
  srv.redirect.status = code;
  srv.redirect.target = tgt.s;
  expect(";", "missing ';' after return");
  return true;
}

bool ConfigParser::parseDirectiveReturn(LocationConfig &loc) {
  if (!accept("return"))
    return false;
  Token st = next();
  Token tgt = next();
  int code = toInt(st.s);
  if (code <= 0 || tgt.s.empty() || tgt.s == "{" || tgt.s == "}" ||
      tgt.s == ";")
    throw ParseError("invalid return directive (location)");
  loc.redirect.status = code;
  loc.redirect.target = tgt.s; // comment retained from previous code
  expect(";", "missing ';' after return");
  return true;
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
