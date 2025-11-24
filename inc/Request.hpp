// Request.hpp

#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "Enums.hpp"

class Response;

// stores and validates (semantically) an HTTP request
class Request {
  public:
    // Constructors
    Request(void);
    ~Request(void);

    // Semantic validation
    requestValidity validateRequest(Response& res) const;
    // performs all the static (not config based) checks to set the _validity

    // Other functions
    bool hasHeader(requestHeaders); // whether a specific header is present
    bool hasBody(void);

    // getters
    requestMethod      getMethod(void) const;
    const std::string& getPath(void) const;
    const std::string& getQueryString(void) const;
    const std::string& getProtocolVersion(void) const;
    const headersMap&  getHeaders(void) const;
    const std::string  getHeader(requestHeaders) const;
    const std::string& getBody(void) const;
    requestValidity    getValidity(void) const;
    statusCode         getStatusCode(void) const;
    void               resolveAbsolutePath(std::string& path);
    // setters
    void setMethod(const std::string&);
    void setPath(std::string const&);
    void setQueryString(std::string const&);
    void setProtocolVersion(const std::string&);
    void setHeaders(const headersMap&);
    void setHeader(requestHeaders, const std::string&);
    void setBody(std::string const&);
    void setValidity(requestValidity);
    void setStatusCode(statusCode);

  private:
    // Attributes
    requestMethod _method;
    std::string   _path;
    std::string   _queryString;
    std::string   _protocolVersion;

    headersMap      _headers;
    std::string     _body;
    statusCode      _statusCode;
    requestValidity _validity;
};

std::ostream& operator<<(std::ostream&, const Request&);
