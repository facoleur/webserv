// Server.hpp

#pragma once

#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
// #include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <queue>
#include <vector>

#include "Request.hpp"
#include "RequestParser.hpp"
#include "Response.hpp"
#include "Webserv.hpp"

class Response;

#define MAX_EVENTS 64
#define TIMEOUT 2500
#define READ_SIZE 10

struct ClientContext {
    RequestParser        req_parser;
    std::queue<Request>  requests;
    std::queue<Response> responses;
};

class Server {
  private:
    // Config               _config;

  public:
    Server();
    ~Server();

    Response        process_request(Request& request);
    void            new_connection();
    void            existing_connection();
    void            run();
    requestValidity handle_requests(ClientContext& context, int cfd);
    void            print_request();
    void            disconnect_client(int& index, int& client_fd, struct pollfd* pfds, int& nfds);
};
