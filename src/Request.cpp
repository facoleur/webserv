// Request.cpp

#include "Request.hpp"

Request::Request(void)
    : _method(UNKNOWN), _path(), _queryString(), _protocolVersion(), _body(), _statusCode(NO_STATUS),
      _validity(INVALID_REQUEST) {
}

Request::~Request(void) {
}

void Request::printRequest(void) const {
    std::cout << "path: " << _path << std::endl;
    std::cout << "method: " << _method << std::endl;
    std::cout << "body: " << _body << std::endl;

    for (std::map<enum requestHeaders, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        std::cout << "header: " << (*it).first << ": " << (*it).second << std::endl;
    }

    std::cout << _body << std::endl;
}

enum requestMethod Request::getMethod(void) const {
    return _method;
}

std::string Request::getPath(void) const {
    return _path;
}

const std::string& Request::getQueryString(void) const {
    return _queryString;
}

const std::string& Request::getProtocolVersion(void) const {
    return _protocolVersion;
}

const std::map<enum requestHeaders, std::string>& Request::getHeaders(void) const {
    return _headers;
}

std::string Request::getHeader(enum requestHeaders headers) const {
    for (std::map<enum requestHeaders, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        if (headers == (*it).first) {
            std::string res = (*it).second;
            return res;
        }
    }
    return "";
}

const std::string& Request::getBody(void) const {
    return _body;
}

enum requestValidity Request::getValidity(void) {
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

void Request::setQueryString(std::string const& queryString) {
    _queryString = queryString;
}

void Request::setProtocolVersion(std::string& protocolVersion) {
    _protocolVersion = protocolVersion;
}

void Request::setValidity(enum requestValidity val) {
    _validity = val;
}

void Request::setStatusCode(enum statusCode val) {
    _statusCode = val;
}

// void setHeaders(std::vector<std::string, std::string>);
// void setBody(std::string &);

void Request::validateRequest(void) // performs all the necessary checks to set the _validity
{
    if (!validateMethod()) {
        std::cout << "Request: couldn't validate method" << std::endl;
        return (setStatusCode(NOT_IMPLEMENTED));
    }
    // if (!validateTarget())
    //     return; // n.b.: various possible error codes, set by validateTarget()
    // if (!validateQueryString())
    //     return; // ?
    if (!validateProtocolVersion()) {
        std::cout << "Request: couldn't validate protocol version" << std::endl;
        return (setStatusCode(HTTP_VERSION_NOT_SUPPORTED));
    }
    // if (!validateHeaders())
    //     return; // ?
    // if (!validateBody())
    //     return; // ?
    _validity = VALID_REQUEST;
}

// validity checks => semantic validation
bool Request::validateMethod(void) {
    if (_method == GET || _method == POST || _method == DELETE)
        return true;
    return false;
}

// bool Request::validateTarget(void) {
// }

// bool Request::validateQueryString(void) {
//     if (_queryString.empty())
//         return true;
//     // ...
// }

bool Request::validateProtocolVersion(void) {
    if (_protocolVersion == "HTTP/1.0" || _protocolVersion == "HTTP/1.1")
        return true;
    else
        return false;
}

// bool Request::validateHeaders(void) {
// }

// bool Request::validateBody(void) {
// }

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
