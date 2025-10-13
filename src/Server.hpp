// Server.hpp

#pragma once

#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Config.hpp"

#define MAX_EVENTS 64
#define TIMEOUT 30000
#define READ_SIZE 10

class Server {
private:
  Config conf;

public:
  Server(Config &conf);

  void run();

  ~Server();
};
