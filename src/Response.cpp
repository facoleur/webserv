// Response.cpp

#include "Response.hpp"
#include "Request.hpp"
#include "Utils.hpp"

std::map<enum requestHeaders, std::string> Headers::_headersString;

Response::Response() : _statusCode(OK) {
}

Response::Response(enum statusCode statusCode) : _statusCode(statusCode) {
}

Response::~Response() {
}

std::string& Response::serialize() {
    _serializedResponse.clear();

    _serializedResponse.append("HTTP/1.1 ");
    _serializedResponse.append(toString(_statusCode) + " ");
    _serializedResponse.append(ReasonPhrase::get(_statusCode));
    _serializedResponse.append("\r\n");

    for (std::map<enum requestHeaders, std::string>::iterator it = _headers.begin(); it != _headers.end(); it++) {
        _serializedResponse.append(Headers::getHeader(it->first));
        _serializedResponse.append(": ");
        _serializedResponse.append(it->second);
        _serializedResponse.append("\r\n");
    }
    for (std::map<std::string, std::string>::iterator it = _customHeaders.begin(); it != _customHeaders.end(); ++it) {
        _serializedResponse.append(it->first);
        _serializedResponse.append(": ");
        _serializedResponse.append(it->second);
        _serializedResponse.append("\r\n");
    }
    _serializedResponse.append("\r\n");
    _serializedResponse.append(_body);

    return _serializedResponse;
}

void Response::setStatusCode(enum statusCode statusCode) {
    _statusCode = statusCode;
}

void Response::setHeader(enum requestHeaders key, const std::string& value) {
    _headers[key] = value;
}

void Response::setHeaders(const std::map<enum requestHeaders, std::string>& headers) {
    for (std::map<enum requestHeaders, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        _headers[it->first] = it->second;
    }
}

void Response::setBody(const std::string& body) {
    _body = body;
}

enum statusCode Response::getStatusCode() const {
    return _statusCode;
}

const std::map<enum requestHeaders, std::string>& Response::getHeaders() const {
    return _headers;
}

const std::string& Response::getHeader(enum requestHeaders header) const {
    return _headers.at(header);
}

const std::string& Response::getBody() const {
    return _body;
}

bool Response::isError() {
    return _statusCode >= 400;
}
