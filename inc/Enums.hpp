// Enums.hpp

#pragma once

#include <map>
#include <string>

enum requestHeaders { HOST, CONTENT_LENGTH, LOCATION, TRANSFER_ENCODING, CONTENT_TYPE, CONNECTION, ACCEPT };

enum requestMethod { GET, POST, DELETE, UNKNOWN };

enum statusCode {
    NO_STATUS = 0,

    // Success
    OK         = 200,
    CREATED    = 201,
    ACCEPTED   = 202,
    NO_CONTENT = 204,

    // Redirections (3xx)
    REDIRECT = 301,

    // Errors
    BAD_REQUEST                = 400,
    FORBIDDEN                  = 403, // probably not used; discussed on 12/11 call
    NOT_FOUND                  = 404,
    NOT_ALLOWED                = 405, // checked if relevant => yes (also in Kaydoo's)
    LENGTH_REQUIRED            = 411,
    CONTENT_TOO_LARGE          = 413,
    INTERNAL_SERVER_ERROR      = 500,
    NOT_IMPLEMENTED            = 501,
    BAD_GATEWAY                = 502,
    HTTP_VERSION_NOT_SUPPORTED = 505
};

typedef std::map<requestHeaders, std::string> headersMap;

enum ParserState { REQ_PARSE_START, REQ_PARSE_PARTIAL, REQ_PARSE_COMPLETE, REQ_PARSE_ERROR };

enum ParsingPhase { PARSING_START_LINE, PARSING_HEADERS, PARSING_BODY, PARSING_COMPLETE };
