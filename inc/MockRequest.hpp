// MockRequest.hpp

#pragma once

#include <iostream>
#include <map>
#include <utility>

enum requestMethod { GET, POST, DELETE, UNKNOWN };
enum requestValidity { VALID_REQUEST, INVALID_REQUEST };
enum requestHeaders { HOST, CONTENT_LENGTH, TRANSFER_ENCODING, CONTENT_TYPE, CONNECTION, ACCEPT };

class MockRequest {
  private:
    std::string                                _path;
    enum requestMethod                         _method;
    std::string                                _queryString;
    std::map<enum requestHeaders, std::string> _headers;
    std::string                                _body;
    int                                        _protocolVersion;

  public:
    MockRequest();
    ~MockRequest();

    enum requestMethod                          getMethod(void);
    std::string&                                getPath(void);
    std::string&                                getQueryString(void);
    int                                         getProtocolVersion(void);
    std::map<enum requestHeaders, std::string>& getHeaders(void);
    std::string&                                getBody(void);
    bool                                        isRequestValid(void);

    void setMethod(enum requestMethod method);
    void setPath(const std::string& path);
    void setQueryString(const std::string& queryString);
    void setProtocolVersion(int version);
    void setHeaders(const std::map<enum requestHeaders, std::string>& headers);
    void setHeader(enum requestHeaders header, const std::string& value);
    void setBody(const std::string& body);
};

