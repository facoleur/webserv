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

const std::map<enum requestHeaders, std::string> Request::getHeaders(void) const const {
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
    _headers[key] = value;
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

void Request::setProtocolVersion(std::string& protocolVersion) {
    _protocolVersion = protocolVersion;
}

void Request::setValidity(enum requestValidity val) {
    _validity = val;
}

void Request::setStatusCode(enum statusCode val) {
    _statusCode = val;
}

// performs all the static (not config based) checks to set the _validity
enum requestValidity Request::validateRequest(Response& res) const {
    if (!validateMethod()) {
        std::cout << "Request: couldn't validate method" << std::endl;
        res.setStatusCode(NOT_IMPLEMENTED);
        return INVALID_REQUEST;
    }
    if (!validateTarget() || !validateQueryString()) {
        res.setStatusCode(BAD_REQUEST);
        return INVALID_REQUEST; // n.b.: various possible error codes, set by validateTarget()
    }
    
    if (!validateProtocolVersion()) {
        // must be HTTP/1.1
        res.setStatusCode(HTTP_VERSION_NOT_SUPPORTED);
        return INVALID_REQUEST;
    }

    if (!validateHeaders()) {
        // must have exactly one [host] header
        // more than 1 header contentlength: has coma => bad request (non neg, integer) 
        // transfer-encoding => must be "chunked", must not contain content-length. If wrong: 501
        res.setStatusCode(BAD_REQUEST);
        // res.setStatusCode(NOT_IMPLEMENTED);
        return INVALID_REQUEST;
    }
    if (!validateBody()) {
        // has no body if POST => invalid
        // has body if GET => invalid
        // move validation of content length == body.size
        res.setStatusCode(BAD_REQUEST);
        return INVALID_REQUEST;
    }
    DEBUG_LOG("VALID!");
    return VALID_REQUEST;
}

// validity checks => semantic validation
bool Request::validateMethod(void) const {
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
