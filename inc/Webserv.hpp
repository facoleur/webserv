// Webserv.hpp
// General includes

#pragma once

#include <iostream>
#include <string>

enum statusCode {
    NO_STATUS = 0,

    // Success
    OK         = 200,
    ACCEPTED   = 202,
    NO_CONTENT = 204,

    // Redirections (3xx)

    // Errors
    BAD_REQUEST                = 400,
    FORBIDDEN                  = 403,
    NOT_FOUND                  = 404,
    NOT_ALLOWED                = 405, // checked if relevant => yes (also in Kaydoo's)
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
