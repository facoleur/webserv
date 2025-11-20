// Response.hpp

#pragma once

#include <iostream>
#include <map>
#include <utility>

#include "Enums.hpp"
#include "Request.hpp"
#include "Webserv.hpp"

class Headers {
  private:
    static headersMap _headersMap;

    static void init() {
        _headersMap[HOST]              = "Host";
        _headersMap[CONTENT_LENGTH]    = "Content-Length";
        _headersMap[LOCATION]          = "Location";
        _headersMap[TRANSFER_ENCODING] = "Transfer-Encoding";
        _headersMap[CONTENT_TYPE]      = "Content-Type";
        _headersMap[CONNECTION]        = "Connection";
        _headersMap[ACCEPT]            = "Accept";
    }

  public:
    Headers();
    ~Headers();

    static std::string getHeader(requestHeaders header) {
        init();
        return _headersMap[header];
    }
};

class ReasonPhrase {
  public:
    static const char* get(statusCode code) {
        switch (code) {
            case 200:
                return "OK";
            case 201:
                return "Created";
            case 202:
                return "Accepted";
            case 204:
                return "No Content";
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
            case 411:
                return "Length Required";
            case 413:
                return "Payload Too Large";
            case 500:
                return "Server Error";
            case 501:
                return "Not Implemented";
            case 502:
                return "Bad Gateway";
            case 505:
                return "Http Version Not Supported";
            default:
                return "Unknown";
        }
    }
};

class Response {
  private:
    statusCode  _statusCode;
    headersMap  _headers;
    std::string _body;

    std::string _serializedResponse;

  public:
    Response();
    Response(statusCode statusCode);
    ~Response();

    void               setStatusCode(statusCode);
    void               setHeader(requestHeaders, const std::string&);
    void               setHeaders(const headersMap&);
    void               setBody(const std::string&);
    statusCode         getStatusCode() const;
    const std::string& getHeader(requestHeaders) const;
    const headersMap&  getHeaders() const;
    const std::string& getBody() const;
    bool               isError();

    std::string& serialize();

    friend std::ostream& operator<<(std::ostream&, const Response&);
};
