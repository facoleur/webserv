// Request.hpp

#pragma once

#include "Response.hpp"
#include "Webserv.hpp"
#include <map>
#include <vector>

enum requestMethod;
enum requestHeaders;

enum requestValidity { INVALID_REQUEST, VALID_REQUEST };

class Response;

// stores and validates (semantically) an HTTP request
class Request {
  public:
    // Constructors
    Request(void);
    ~Request(void);

    // Semantic validation
    enum requestValidity validateRequest(Response& res) const;
    // performs all the static (not config based) checks to set the _validity

    bool isValidMethod(void) const;
    bool isValidTarget(void) const;
    bool isValidQueryString(void) const;
    bool isValidProtocolVersion(void) const;
    bool isValidHeaders(Response&) const;
    bool isValidBody(void) const;

    // Other functions
    bool hasHeader(std::string const&); // whether a specific header is present
    bool hasBody(void);
    void printRequest(void) const;

    // getters
    requestMethod                                     getMethod(void) const;
    const std::string&                                getPath(void) const;
    const std::string&                                getQueryString(void) const;
    const std::string&                                getProtocolVersion(void) const;
    const std::map<enum requestHeaders, std::string>& getHeaders(void) const;
    const std::string                                 getHeader(enum requestHeaders) const;
    const std::string&                                getBody(void) const;
    requestValidity                                   getValidity(void) const;
    statusCode                                        getStatusCode(void) const;
    void                                              resolveAbsolutePath(std::string& path);
    // setters
    void setMethod(const std::string&);
    void setPath(std::string const&);
    void setQueryString(std::string const&);
    void setProtocolVersion(const std::string&);
    void setHeaders(const std::map<enum requestHeaders, std::string>&);
    void setHeader(enum requestHeaders, const std::string&);
    void setBody(std::string const&);
    void setValidity(requestValidity);
    void setStatusCode(statusCode);

    friend std::ostream& operator<<(std::ostream&, requestMethod);
    friend std::ostream& operator<<(std::ostream&, requestValidity);
    friend std::ostream& operator<<(std::ostream&, statusCode);
    friend std::ostream& operator<<(std::ostream&, const Request&);
    void                 printHeaders(std::ostream&) const;

  private:
    // Attributes
    requestMethod _method;
    std::string   _path;
    std::string   _queryString;
    std::string   _protocolVersion;

    std::map<enum requestHeaders, std::string> _headers;
    std::string                                _body;
    statusCode                                 _statusCode;
    requestValidity                            _validity;
};

// std::ostream& operator<<(std::ostream& os, Request& req);
std::ostream& operator<<(std::ostream& os, const Request& req);
