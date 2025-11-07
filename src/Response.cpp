// Response.cpp

#include "Response.hpp"
#include "Request.hpp"
#include "Utils.hpp"

std::map<int, std::string>                 ReasonPhrase::_reasonPhrase;
std::map<enum requestHeaders, std::string> Headers::_headersString;

Response::Response() : _statusCode(200) {
}

Response::Response(int statusCode) : _statusCode(statusCode) {
}

Response::~Response() {
}

std::string& Response::serialize() {
    _serializedResponse.clear();

    _serializedResponse.append("HTTP/1.1 ");
    _serializedResponse.append(toString(_statusCode) + " ");
    _serializedResponse.append(ReasonPhrase::getReasonPhrase(_statusCode) + "\r\n");

    for (std::map<enum requestHeaders, std::string>::iterator it = _headers.begin(); it != _headers.end(); it++) {
        _serializedResponse.append(Headers::getHeader(it->first));
        _serializedResponse.append(": ");
        _serializedResponse.append(it->second);
        _serializedResponse.append("\r\n");
    }
    _serializedResponse.append("\r\n");
    _serializedResponse.append(_body);

    return _serializedResponse;
}
