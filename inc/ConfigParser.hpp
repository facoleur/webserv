#pragma once

#include "Config.hpp"
#include <stdexcept>
#include <string>
#include <vector>

class ParseError : public std::runtime_error {
  public:
    ParseError(const std::string& msg) : std::runtime_error(msg) {
    }
};

class ConfigParser {
  public:
    ConfigParser();
    Config parseFile(const std::string& path);
    Config parseString(const std::string& text);

  private:
    struct Token {
        std::string s;
        size_t      line; // For error msg
        size_t      col;  // For error msg
    };
    std::vector<Token> tokenize(const std::string& text);

    void parseConfig();
    void parseServer();
    void parseLocation(ServerConfig& srv);
    // Reusable directive handlers (no behavior change vs previous code)
    bool parseDirectiveRoot(ServerConfig& srv);
    bool parseDirectiveRoot(LocationConfig& loc);
    bool parseDirectiveIndex(ServerConfig& srv);
    bool parseDirectiveIndex(LocationConfig& loc);
    bool parseDirectiveMethods(ServerConfig& srv);
    bool parseDirectiveMethods(LocationConfig& loc);
    bool parseDirectiveReturn(ServerConfig& srv);
    bool parseDirectiveReturn(LocationConfig& loc);
    bool parseDirectiveListen(ServerConfig& srv);
    bool parseDirectiveErrorPage(ServerConfig& srv);
    bool parseDirectiveAutoIndex(ServerConfig& srv);
    bool parseDirectiveAutoIndex(LocationConfig& loc);
    bool parseDirectiveClientMaxBodySize(ServerConfig& srv);
    bool parseDirectiveClientMaxBodySize(LocationConfig& loc);
    bool parseDirectiveCgi(ServerConfig& srv);
    bool parseDirectiveCgi(LocationConfig& loc);
    bool parseDirectiveUploadEnable(LocationConfig& loc);
    bool parseDirectiveUploadStore(LocationConfig& loc);

    bool         accept(const std::string& kw);
    void         expect(const std::string& kw, const char* err);
    Token        next();       // consomme
    const Token& peek() const; // sans consommer
    bool         eof() const;

    static bool isMethod(const std::string& m);
    static int  toInt(const std::string& s);

    Config             cfg_;
    std::vector<Token> toks_;
    size_t             i_;
};
