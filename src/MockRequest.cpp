// MockRequest.cpp

#include "MockRequest.hpp"

MockRequest::MockRequest() {
    _path            = "/about";
    _queryString     = "id=42";
    _body            = "mock body content";
    _protocolVersion = 11;

    _headers.insert(std::make_pair(HOST, "localhost:8080"));
    _headers.insert(std::make_pair(CONTENT_LENGTH, "2000"));
    _headers.insert(std::make_pair(TRANSFER_ENCODING, "chunked"));
    _headers.insert(std::make_pair(CONTENT_TYPE, "text/html"));
    _headers.insert(std::make_pair(CONNECTION, "keep-alive"));
    _headers.insert(std::make_pair(ACCEPT, "*/*"));
}

MockRequest::~MockRequest() {
}

void MockRequest::setMethod(enum requestMethod method) {
    _method = method;
}

void MockRequest::setPath(const std::string& path) {
    _path = path;
}

void MockRequest::setQueryString(const std::string& queryString) {
    _queryString = queryString;
}

void MockRequest::setProtocolVersion(int version) {
    _protocolVersion = version;
}

void MockRequest::setHeaders(const std::map<enum requestHeaders, std::string>& headers) {
    _headers = headers;
}

void MockRequest::setHeader(enum requestHeaders header, const std::string& value) {
    _headers[header] = value;
}

void MockRequest::setBody(const std::string& body) {
    _body = body;
}
enum requestMethod MockRequest::getMethod(void) {
    return _method;
}
std::string& MockRequest::getPath(void) {
    return _path;
}
std::string& MockRequest::getQueryString(void) {
    return _queryString;
}
int MockRequest::getProtocolVersion(void) {
    return _protocolVersion;
}
std::map<enum requestHeaders, std::string>& MockRequest::getHeaders(void) {
    return _headers;
}
std::string& MockRequest::getBody(void) {
    return _body;
}
bool MockRequest::isRequestValid(void) {
    return true;
}
