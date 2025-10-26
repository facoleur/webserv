// Server.hpp

#pragma once

#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <queue>
#include <vector>

#include "Request.hpp"
#include "RequestParser.hpp"
#include "Response.hpp"

class Response;

#define MAX_EVENTS 64
#define TIMEOUT 2500
#define READ_SIZE 10

struct ClientContext {
    RequestParser       req_parser;
    std::queue<Request> requests;
};

class Server {
  private:
    // Config               _config;

  public:
    Server();
    ~Server();
    void     new_connection();
    void     existing_connection();
    void     run();
    Response process_request(Request& request);
    void     handle_requests(ClientContext& req, int cfd);

    void print_request();
    void disconnect_client(int& index, int& client_fd, struct pollfd* pfds, int& nfds);
};
