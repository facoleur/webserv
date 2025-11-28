// RequestParserBody.cpp

#include "Request.hpp"
#include "RequestParser.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

// case CONTENT_LENGTH:
// accumulator => parser _body buffer (under constraint maxBodySize) => request

void RequestParser::parseBody(Request& req, size_t maxBodySize) {
    (void)maxBodySize; // maybe used for chunk parsing, to see later
    req.setBody(_bodyBuffer);
    DEBUG_LOG("parseBody set body to " + req.getBody());
}
