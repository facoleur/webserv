// Server.hpp

#pragma once

#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <queue>
#include <vector>

#include "MockResponse.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "Config.hpp"
#include "utils.hpp"

class MockResponse;

#define MAX_EVENTS 64
#define TIMEOUT 2500
#define READ_SIZE 10

enum ClientState { WAITING, READING, WRITING, CLOSED };

struct ClientContext {
    ClientState         state;
    RequestParser       req_parser;
    std::queue<Request> requests;
    MockResponse        response;
    size_t              server_index; // which server accepted the client
};

class Server {
  private:
    const Config*        _cfg; // not owning pointer
    struct ClientContext _state;
    std::map<int, size_t> _listenerToServerIdx; // listen fd -> server index

  public:
    Server();
    Server(const Config& cfg);
    ~Server();
    void         new_connection();
    void         existing_connection();
    void         run();
    MockResponse process_request(Request& request);
    void         handle_requests(ClientContext& req, int cfd);

    void print_request();
    void disconnect_client(int& index, int& client_fd, struct pollfd* pfds, int& nfds);
};
