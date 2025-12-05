// Request.hpp

#pragma once

#include <iostream>

#include "CGI.hpp"
#include "Enums.hpp"

struct ClientContext;
class Response;

// stores and validates (semantically) an HTTP request
class Request {
  public:
    // Constructors
    Request(void);
    Request(ClientContext&, int);
    ~Request(void);

    CgiInfo cgiInfo;

    // Other functions
    bool hasHeader(requestHeaders); // whether a specific header is present

    // getters
    requestState       getState(void) const;
    ClientContext*     getClientPtr(void);
    ClientContext&     getClientContext(void) const;
    int                getClientFd(void) const;
    requestMethod      getMethod(void) const;
    const std::string& getPath(void) const;
    const std::string& getQueryString(void) const;
    const std::string& getProtocolVersion(void) const;
    const headersMap&  getHeaders(void) const;
    const std::string  getHeader(requestHeaders) const;
    const std::string& getBody(void) const;
    statusCode         getStatusCode(void) const;
    void               resolveAbsolutePath(std::string& path);

    // setters
    void setState(requestState);
    void setClientFd(int);
    void setMethod(const std::string&);
    void setPath(std::string const&);
    void setQueryString(std::string const&);
    void setProtocolVersion(const std::string&);
    void setHeaders(const headersMap&);
    void setHeader(requestHeaders, const std::string&);
    void setBody(std::string const&);
    void setStatusCode(statusCode);

  private:
    // Attributes
    requestState   _state;
    ClientContext* _client;
    int            _clientFd;
    requestMethod  _method;
    std::string    _path;
    std::string    _queryString;
    std::string    _protocolVersion;

    headersMap  _headers;
    std::string _body;
    statusCode  _statusCode;
};

std::ostream& operator<<(std::ostream&, const Request&);
