// Response.hpp

#pragma once

#include <iostream>
#include <map>
#include <utility>

#include "Request.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

class Headers {
  private:
    static std::map<enum requestHeaders, std::string> _headersString;

    static void init() {
        _headersString[HOST]              = "Host";
        _headersString[CONTENT_LENGTH]    = "Content-Length";
        _headersString[LOCATION]          = "Location";
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
class ReasonPhrase {
  public:
    static const char* get(enum statusCode code) {
        switch (code) {
            case 200:
                return "OK";
            case 202:
                return "Accepted";
            case 204:
                return "Accepted";
            case 301:
                return "Redirect";
            case 400:
                return "Bad Request";
            case 403:
                return "Forbidden";
            case 404:
                return "Not Found";
            case 405:
                return "Method Not Allowed";
            case 500:
                return "Server Error";
            case 501:
                return "Not Implemented";
            case 505:
                return "Http Version Not Supported";
            default:
                return "Unknown";
        }
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

    void setStatusCode(enum statusCode);
    void setHeader(enum requestHeaders, const std::string&);
    void setHeaders(const std::map<enum requestHeaders, std::string>&);
    void setBody(const std::string&);

    enum statusCode                                   getStatusCode() const;
    const std::string&                                getHeader(enum requestHeaders) const;
    const std::map<enum requestHeaders, std::string>& getHeaders() const;
    const std::string&                                getBody() const;

    std::string& serialize();
};
