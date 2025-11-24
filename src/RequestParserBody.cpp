// RequestParserBody.cpp

#include "Enums.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "Utils.hpp"

bool RequestParser::isValidBody(Request& req) const { // not called yet
    // move validation of content length == body.size
    if (req.getMethod() == POST && req.getBody().empty())
        return false;

    if (req.getMethod() == GET && !req.getBody().empty())
        return false;

    if (req.getBody().size() != toSizet(req.getHeader(CONTENT_LENGTH))) {
        DEBUG_LOG("req.getBody().size(): " + toString(req.getBody().size()) + " != content-length(" +
                  toString(toSizet(req.getHeader(CONTENT_LENGTH))));
        return false;
    }
    return true;
}
