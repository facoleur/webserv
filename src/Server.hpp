// Server.hpp

#pragma once

#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "Config.hpp"

class MockResponse;

#define MAX_EVENTS 64
#define TIMEOUT 2500
#define READ_SIZE 10

class Server {
private:
  Config conf;

public:
  Server(Config &conf);

  void run();
  MockResponse process_request(std::string &request);
  void print_request();
  void disconnect_client(int &index, int &client_fd, struct pollfd *pfds,
                         int &nfds);
  ~Server();
};
