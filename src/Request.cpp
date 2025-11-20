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

// std::vector<std::string, std::string> Request::getHeaders(void) const {
//     return _headers;
// }

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

// performs all the static (not config based) checks to set the _validity
enum requestValidity Request::validateRequest(Response& res) const {
    if (!isValidMethod()) {
        res.setStatusCode(NOT_IMPLEMENTED);
        return INVALID_REQUEST;
    }

    if (!isValidTarget() || !isValidQueryString()) {
        res.setStatusCode(BAD_REQUEST);
        return INVALID_REQUEST;
    }

    if (!isValidProtocolVersion()) {
        // must be HTTP/1.1
        res.setStatusCode(HTTP_VERSION_NOT_SUPPORTED);
        return INVALID_REQUEST;
    }

    if (!isValidHeaders(res)) {
        // must have exactly one [host] header
        // more than 1 header contentlength: has coma => bad request (non neg, integer)
        // transfer-encoding => must be "chunked", must not contain content-length. If wrong: 501
        // res.setStatusCode(NOT_IMPLEMENTED);
        return INVALID_REQUEST;
    }
    if (!isValidBody()) {
        // has no body if POST => invalid
        // has body if GET => invalid
        // move validation of content length == body.size
        res.setStatusCode(BAD_REQUEST);
        return INVALID_REQUEST;
    }
    DEBUG_LOG("validateRequest: VALID until now");
    return VALID_REQUEST;
}

void Request::resolveAbsolutePath(std::string& path) {
    std::string::size_type pos = path.find("http://");
    path.erase(pos, 7);

    pos = path.find("/");
    path.erase(0, pos);
}

// validity checks => semantic validation
bool Request::isValidMethod(void) const {
    if (_method == GET || _method == POST || _method == DELETE)
        return true;
    return false;
}

bool Request::isValidTarget(void) const {
    if (!startsWith(_path, "http://") && !startsWith(_path, "/")) {
        return false;
    }

    if (_path.find("%") != std::string::npos || _path.find("=") != std::string::npos ||
        _path.find("../") != std::string::npos || _path.find("./") != std::string::npos ||
        _path.find("=") != std::string::npos || _path.find("&") != std::string::npos)
        return false;

    return true;
}

bool Request::isValidQueryString(void) const {
    bool isvalid = true;
    for (size_t i = 0; i < _queryString.size(); ++i) {
        if (std::isalnum(static_cast<unsigned char>(_queryString[i])))
            return true;
        const std::string allowed = "_-./?&=%";
        isvalid                   = (allowed.find(_queryString[i]) != std::string::npos);
    }
    return isvalid;
}

bool Request::isValidProtocolVersion(void) const {
    return _protocolVersion == "HTTP/1.1";
}

bool Request::isValidHeaders(Response& res) const {
    // must have exactly one [host] header
    // more than 1 header contentlength: has coma => bad request (non neg, integer)
    // transfer-encoding => must be "chunked", must not contain content-length. If wrong: 501
    bool isvalid = true;

    if (_headers.find(CONTENT_LENGTH) != _headers.end() && _headers.at(CONTENT_LENGTH).find(",") != std::string::npos) {
        isvalid = false;
        res.setStatusCode(BAD_REQUEST);
    }

    if (_headers.find(TRANSFER_ENCODING) != _headers.end()) {
        isvalid = _headers.at(TRANSFER_ENCODING) == "chunked";
        if (isvalid == false)
            res.setStatusCode(NOT_IMPLEMENTED);
    }
    return isvalid;
}

bool Request::isValidBody(void) const {
    // has no body if POST => invalid
    // has body if GET => invalid
    // move validation of content length == body.size
    if (getBody().empty() && getMethod() == POST)
        return false;

    if (getBody().size() != toSizet(getHeader(CONTENT_LENGTH)))
        return false;

    return true;
}

bool Request::hasHeader(requestHeaders header) {
    if (getHeader(header).empty())
        return false;
    return true;
}
