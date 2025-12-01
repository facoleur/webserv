// Webserv.hpp
// General includes

#pragma once

#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

// Toggle this during compilation with -DDEBUG_MODE
#ifdef DEBUG_MODE
#define DEBUG_LOG(x) std::cout << x << std::endl
#else
#define DEBUG_LOG(x)
#endif
