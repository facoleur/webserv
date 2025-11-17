// Response.hpp

#pragma once

#include <iostream>
#include <map>
#include <utility>

#include "Request.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

class ReasonPhrase {
  private:
    static std::map<int, std::string> _reasonPhrase;

    static void init() {
        _reasonPhrase[200] = "OK";
        _reasonPhrase[202] = "Accepted";
        _reasonPhrase[301] = "Redirect";
        _reasonPhrase[400] = "Bad Request";
        _reasonPhrase[404] = "Not Found";
        _reasonPhrase[500] = "Server Error";
    }

  public:
    ReasonPhrase();
    ~ReasonPhrase();

    static std::string getReasonPhrase(int statusCode) {
        init();
        return _reasonPhrase[statusCode];
    }
};

class Headers {
  private:
    static std::map<enum requestHeaders, std::string> _headersString;

    static void init() {
        _headersString[HOST]              = "Host";
        _headersString[CONTENT_LENGTH]    = "Content-Length";
        _headersString[TRANSFER_ENCODING] = "Transfer-Encoding";
        _headersString[CONTENT_TYPE]      = "Content-Type";
        _headersString[CONNECTION]        = "Connection";
        _headersString[ACCEPT]            = "Accept";
    }

  public:
    Headers();
    ~Headers();

    static std::string getHeader(enum requestHeaders header) {
        init();
        return _headersString[header];
    }
};

class Response {
  private:
    enum statusCode                            _statusCode;
    std::map<enum requestHeaders, std::string> _headers;
    std::string                                _body;

    std::string _serializedResponse;

  public:
    Response();
    Response(enum statusCode statusCode);
    ~Response();

    std::string& serialize();
};
