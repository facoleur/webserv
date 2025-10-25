// Request.cpp

#include "Request.hpp"

Request::Request(void) : _validity(INVALID_REQUEST) {
}

Request::~Request(void) {
}

void Request::mockRequest() {
    _body   = "body";
    _method = GET;
    _path   = "path/";
    _headers.push_back(std::make_pair("key", "val"));
}
