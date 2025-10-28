// Request.hpp

#pragma once

#include "Webserv.hpp"
#include <vector>

enum requestMethod { GET, POST, DELETE, UNKNOWN };
enum requestHeaders { HOST, CONTENT_LENGTH, TRANSFER_ENCODING, CONTENT_TYPE, CONNECTION, ACCEPT };
enum requestValidity { VALID_REQUEST, INVALID_REQUEST };

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
    void printRequest(void);

    // getters
    enum requestMethod                     getMethod(void);
    std::string&                           getPath(void);
    std::string&                           getQueryString(void);
    std::string&                           getProtocolVersion(void);
    std::vector<std::string, std::string>& getHeaders(void);
    std::string&                           getBody(void);
    enum requestValidity                   getValidity(void);

    // setters
    void setMethod(std::string&);
    void setPath(std::string const&);
    void setQueryString(std::string const&);
    void setProtocolVersion(std::string&);
    void setHeaders(std::vector<std::string, std::string>);
    void setBody(std::string const&);
    void setValidity(enum requestValidity);

    friend std::ostream& operator<<(std::ostream& os, Request& req);
    void                 setStatusCode(enum statusCode);
    void                 printHeaders(std::ostream&);

  private:
    // Attributes
    enum requestMethod _method;
    std::string        _path;
    std::string        _queryString;
    std::string        _protocolVersion;

    std::vector<std::pair<std::string, std::string> >
                    _headers; // lferro: use map, more useable for the next user; and append during parsing if duplicate
    std::string     _body;
    enum statusCode _statusCode;
    enum requestValidity _validity;
};

std::ostream& operator<<(std::ostream& os, Request& req);
