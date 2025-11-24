// Request.cpp

#include <iostream>

#include "Request.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

Request::Request(void)
    : _method(UNKNOWN), _path(), _queryString(), _protocolVersion(), _body(), _statusCode(NO_STATUS),
      _validity(INVALID_REQUEST) {
}

Request::~Request(void) {
}

std::ostream& operator<<(std::ostream& os, const Request& req) {
    os << "[REQUEST]" << std::endl;
    os << "--------------------------------" << std::endl;
    os << "Method:           " << req.getMethod() << std::endl;
    os << "Path:             " << req.getPath() << std::endl;
    os << "Query String:     " << req.getQueryString() << std::endl;
    os << "Headers:          " << req.getHeaders() << std::endl;
    os << "Body:             " << req.getBody() << std::endl;
    os << "Protocol version: " << req.getProtocolVersion() << std::endl;
    os << "Status code:      " << req.getStatusCode() << std::endl;
    os << "Validity:         " << req.getValidity() << std::endl;
    os << "--------------------------------" << std::endl;
    return os;
}

enum requestMethod Request::getMethod(void) const {
    return _method;
}

const std::string& Request::getPath(void) const {
    return _path;
}

const std::string& Request::getQueryString(void) const {
    return _queryString;
}

const std::string& Request::getProtocolVersion(void) const {
    return _protocolVersion;
}

statusCode Request::getStatusCode(void) const {
    return _statusCode;
}

const headersMap& Request::getHeaders() const {
    return _headers;
}

const std::string Request::getHeader(enum requestHeaders headers) const {
    for (std::map<enum requestHeaders, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        if (headers == (*it).first) {
            return (*it).second;
        }
    }
    return "";
}

const std::string& Request::getBody(void) const {
    return _body;
}

enum requestValidity Request::getValidity(void) const {
    return _validity;
}

void Request::setMethod(const std::string& method) {
    if (method == "GET")
        _method = GET;
    else if (method == "POST")
        _method = POST;
    else if (method == "DELETE")
        _method = DELETE;
    else
        _method = UNKNOWN;
}

void Request::setPath(std::string const& path) {
    _path = path;
}

void Request::setHeader(enum requestHeaders key, const std::string& value) {
    _headers[key] += value;
}

void Request::setHeaders(const std::map<enum requestHeaders, std::string>& headers) {
    for (std::map<enum requestHeaders, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        setHeader(it->first, it->second);
    }
}

void Request::setBody(std::string const& body) {
    _body = body;
}

void Request::setQueryString(std::string const& queryString) {
    _queryString = queryString;
}

void Request::setProtocolVersion(const std::string& protocolVersion) {
    _protocolVersion = protocolVersion;
}

void Request::setValidity(enum requestValidity val) {
    _validity = val;
}

void Request::setStatusCode(enum statusCode val) {
    _statusCode = val;
}

void Request::resolveAbsolutePath(std::string& path) {
    std::string::size_type pos = path.find("http://");
    path.erase(pos, 7);

    pos = path.find("/");
    path.erase(0, pos);
}

bool Request::hasHeader(requestHeaders header) {
    if (getHeader(header).empty())
        return false;
    return true;
}
