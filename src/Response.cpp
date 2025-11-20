// Response.cpp

#include "Response.hpp"
#include "Request.hpp"
#include "Utils.hpp"

Response::Response() : _statusCode(OK) {
}

Response::Response(statusCode statusCode) : _statusCode(statusCode) {
}

Response::~Response() {
}

headersMap Headers::_headersMap;

std::string& Response::serialize() {
    _serializedResponse.clear();

    _serializedResponse.append("HTTP/1.1 ");
    _serializedResponse.append(toString(_statusCode) + " ");
    _serializedResponse.append(ReasonPhrase::get(_statusCode));
    _serializedResponse.append("\r\n");

    for (headersMap::iterator it = _headers.begin(); it != _headers.end(); it++) {
        _serializedResponse.append(Headers::getHeader(it->first));
        _serializedResponse.append(": ");
        _serializedResponse.append(it->second);
        _serializedResponse.append("\r\n");
    }
    _serializedResponse.append("\r\n");
    _serializedResponse.append(_body);

    return _serializedResponse;
}

void Response::setStatusCode(statusCode statusCode) {
    _statusCode = statusCode;
}

void Response::setHeader(requestHeaders key, const std::string& value) {
    _headers[key] = value;
}

void Response::setHeaders(const headersMap& headers) {
    for (headersMap::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        _headers[it->first] = it->second;
    }
}

void Response::setBody(const std::string& body) {
    _body = body;
}

statusCode Response::getStatusCode() const {
    return _statusCode;
}

const headersMap& Response::getHeaders() const {
    return _headers;
}

const std::string& Response::getHeader(requestHeaders header) const {
    return _headers.at(header);
}

const std::string& Response::getBody() const {
    return _body;
}

bool Response::isError() {
    return _statusCode >= 400;
}

std::ostream& operator<<(std::ostream& os, const Response& res) {
    os << "[RESPONSE]" << std::endl;
    os << "--------------------------------" << std::endl;
    os << "Headers:          " << res._headers << std::endl;
    os << std::endl;
    os << "Body:             " << res._body << std::endl;
    os << "Status code:      " << res._statusCode << std::endl;
    os << "--------------------------------" << std::endl;
    return os;
}
