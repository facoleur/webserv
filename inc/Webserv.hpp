// Webserv.hpp
// General includes

#pragma once

#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#define WEBSERV_VERSION "webserv/1.0.0"

// Toggle this during compilation with -DDEBUG_MODE
// #ifdef DEBUG_MODE
// #define LOG_DEBUG(x) std::cerr << x << std::endl
// #else
// #define LOG_DEBUG(x)
// #endif
