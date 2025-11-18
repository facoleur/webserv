// Webserv.hpp
// General includes

#pragma once

#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

enum requestMethod { GET, POST, DELETE, UNKNOWN };
enum requestHeaders { HOST, CONTENT_LENGTH, LOCATION, TRANSFER_ENCODING, CONTENT_TYPE, CONNECTION, ACCEPT };

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
    HTTP_VERSION_NOT_SUPPORTED = 505
};

// Toggle this during compilation with -DDEBUG_MODE
#ifdef DEBUG_MODE
#define DEBUG_LOG(x) std::cout << x << std::endl
#else
#define DEBUG_LOG(x)
#endif
