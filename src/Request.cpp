// Request.cpp

#include "Request.hpp"

Request::Request(void)
    : _method(), _path(), _queryString(), _protocolVersion(), _headers(), _body(), _statusCode(NO_STATUS),
      _validity(INVALID_REQUEST) {
}

Request::~Request(void) {
}

void Request::printRequest(void) {
    std::cout << "Request: " << std::endl;
    std::cout << "- method: " << _method << std::endl;
    std::cout << "- path: " << _path << std::endl;
    std::cout << "- queryString: " << _queryString << std::endl;
    std::cout << "- protocolVersion: " << _protocolVersion << std::endl;
    // std::cout << "- headers: " << _headers << std::endl;
    // std::cout << "- body: " << _body << std::endl;
    std::cout << "- statusCode: " << _statusCode << std::endl;
    std::cout << "- validity: " << _validity << std::endl;
}

enum requestValidity Request::getValidity(void) {
    return _validity;
}

std::string& Request::getPath(void) {
    return _path;
}

std::string& Request::getQueryString(void) {
    return _queryString;
}

std::string& Request::getProtocolVersion(void) {
    return _protocolVersion;
}

// std::vector<std::string, std::string>& Request::getHeaders(void) {
//     return _headers;
// }

std::string& Request::getBody(void) {
    return _body;
}

enum requestValidity Request::getValidity(void) {
    return _validity;
}

void Request::setMethod(std::string& method) {
    _method = method;
}

void Request::setPath(std::string const& path) {
    _path = path;
}

void Request::setQueryString(std::string const& queryString) {
    _queryString = queryString;
}

void Request::setProtocolVersion(std::string& protocolVersion) {
    _protocolVersion = protocolVersion;
}

void Request::setValidity(enum requestValidity val) {
    _validity = val;
}

void Request::setStatusCode(enum StatusCode val) {
    _statusCode = val;
}

// void setHeaders(std::vector<std::string, std::string>);
// void setBody(std::string &);

void Request::validateRequest(void) // performs all the necessary checks to set the _validity
{
    if (!validateMethod())
        return (setStatusCode(NOT_IMPLEMENTED));
    // if (!validateTarget())
    //     return; // n.b.: various possible error codes, set by validateTarget()
    // if (!validateQueryString())
    //     return; // ?
    if (!validateProtocolVersion())
        return (setStatusCode(HTTP_VERSION_NOT_SUPPORTED));
    // if (!validateHeaders())
    //     return; // ?
    // if (!validateBody())
    //     return; // ?
    _validity = VALID_REQUEST;
}

// validity checks => semantic validation
bool Request::validateMethod(void) {
    if (_method == "GET" || _method == "POST" || _method == "DELETE")
        return true;
    return false;
}

bool Request::validateTarget(void) {
}

bool Request::validateQueryString(void) {
    if (_queryString.empty())
        return true;
    // ...
}

bool Request::validateProtocolVersion(void) {
    if (_protocolVersion == "HTTP/1.0")
        return true;
    else
        return false;
}

bool Request::validateHeaders(void) {
}

bool Request::validateBody(void) {
}

// in class Request OR in handle_requests
// if (INVALID_REQUEST)
// {
// if (req.getmethod() == method not found)
// 		setStatusCode(METHOD_NOT_FOUND)
// if (blabla)
// 		setStatusCode(BLA_BLA)
// else
// 		setStatusCode(BAD_REQUEST)
// }
