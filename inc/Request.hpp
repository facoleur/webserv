// Request.hpp

#pragma once

#include "Webserv.hpp"
#include <map>
#include <vector>

enum requestMethod { GET, POST, DELETE, UNKNOWN };
enum requestHeaders { HOST, CONTENT_LENGTH, LOCATION, TRANSFER_ENCODING, CONTENT_TYPE, CONNECTION, ACCEPT };
enum requestValidity { INVALID_REQUEST, VALID_REQUEST };

// stores and validates (semantically) an HTTP request
class Request {
  public:
    // Constructors
    Request(void);
    ~Request(void);

    // Semantic validation
    void validateRequest(void); // performs all the necessary checks to set the _validity
    bool validateMethod(void);
    bool validateTarget(void);
    bool validateQueryString(void);
    bool validateProtocolVersion(void);
    bool validateHeaders(void);
    bool validateBody(void);

    // Other functions
    bool hasHeader(std::string const&); // whether a specific header is present
    bool hasBody(void);
    void printRequest(void) const;

    // getters
    requestMethod                                getMethod(void) const;
    const std::string&                           getPath(void) const;
    const std::string&                           getQueryString(void) const;
    const std::string&                           getProtocolVersion(void) const;
    const std::vector<std::string, std::string>& getHeaders(void) const;
    const std::string                            getHeader(enum requestHeaders) const;
    const std::string&                           getBody(void) const;
    requestValidity                              getValidity(void) const;
    statusCode                                   getStatusCode(void) const;

    // setters
    void setMethod(const std::string&);
    void setPath(std::string const&);
    void setQueryString(std::string const&);
    void setProtocolVersion(std::string&);
    void setHeaders(const std::map<enum requestHeaders, std::string>&);
    void setHeader(enum requestHeaders, const std::string&);
    void setBody(std::string const&);
    void setValidity(requestValidity);
    void setStatusCode(statusCode);

    friend std::ostream& operator<<(std::ostream&, requestMethod);
    friend std::ostream& operator<<(std::ostream&, requestValidity);
    friend std::ostream& operator<<(std::ostream&, statusCode);
    friend std::ostream& operator<<(std::ostream&, Request&);
    void                 printHeaders(std::ostream&);

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

std::ostream& operator<<(std::ostream& os, Request& req);
