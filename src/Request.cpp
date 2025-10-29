// Request.cpp

#include "Request.hpp"

Request::Request(void)
    : _method(UNKNOWN), _path(), _queryString(), _protocolVersion(), _headers(), _body(), _statusCode(NO_STATUS),
      _validity(INVALID_REQUEST) {
}

Request::~Request(void) {
}

enum requestMethod Request::getMethod(void) {
    return _method;
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
    if (!validateMethod())
	{
		std::cout << "Request: couldn't validate method" << std::endl;	
		return (setStatusCode(NOT_IMPLEMENTED));
	}
    // if (!validateTarget())
    //     return; // n.b.: various possible error codes, set by validateTarget()
    // if (!validateQueryString())
    //     return; // ?
    if (!validateProtocolVersion())
	{
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
