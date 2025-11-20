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

// Toggle this during compilation with -DDEBUG_MODE
#ifdef DEBUG_MODE
#define DEBUG_LOG(x) std::cout << x << std::endl
#else
#define DEBUG_LOG(x)
#endif
