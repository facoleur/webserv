// Enums.hpp

#pragma once

#include <map>
#include <string>

enum requestState { PENDING, CGI_START, CGI_STREAMING, CGI_DONE };

enum requestHeaders {
    SERVER,
    DATE,
    HOST,
    CONTENT_LENGTH,
    LOCATION,
    TRANSFER_ENCODING,
    CONTENT_TYPE,
    CONNECTION,
    ACCEPT
};

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
    FORBIDDEN                  = 403,
    NOT_FOUND                  = 404,
    NOT_ALLOWED                = 405,
    LENGTH_REQUIRED            = 411,
    CONTENT_TOO_LARGE          = 413,
    INTERNAL_SERVER_ERROR      = 500,
    NOT_IMPLEMENTED            = 501,
    BAD_GATEWAY                = 502,
    GATEWAY_TIMEOUT            = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505
};

typedef std::map<requestHeaders, std::string> headersMap;

// externally-relevant state of the parser
enum ParserState { REQ_PARSE_START, REQ_PARSE_PARTIAL, REQ_PARSE_COMPLETE, REQ_PARSE_ERROR };

// internal state of the parser
enum ParsingPhase {
    PARSING_START_LINE,
    PARSING_HEADERS,
    PARSING_BODY_CONTENT_LENGTH,
    PARSING_BODY_CHUNKED,
    PARSING_BODY_FINISHED,
    PARSING_COMPLETE
};

enum CgiPipeRole { CGI_STDIN, CGI_STDOUT };
