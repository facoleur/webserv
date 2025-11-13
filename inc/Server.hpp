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

#include "Config.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

class Response;

#define MAX_EVENTS 64
#define TIMEOUT 0 //2500
#define READ_SIZE 8000


struct ClientContext {
  RequestParser       req_parser;
  std::queue<Request> requests;
  struct pollfd       pfd;
  std::string         write_buffer;
  size_t              server_index; // which server accepted the client
};

typedef std::map<int, struct ClientContext> ContextMap;

class Server {
  private:
    const Config*         _cfg; // not owning pointer
    struct ClientContext  _state;
    std::map<int, size_t> _listenerToServerIdx; // listen fd -> server index

  public:
    Server();
    Server(const Config& cfg);
    ~Server();

    Response          process_request(Request&);
    void              new_connection();
    void              existing_connection();
    void              run();
    requestValidity   handle_requests(ClientContext&, struct pollfd&);
    void              print_request();
    void              disconnect_client(int&, int&, struct pollfd (&pfds)[MAX_EVENTS], int&, std::map<int, ClientContext>&);
    void              setPollFd(struct pollfd&, int, short, short);
    std::vector<int>  initListenerSockets(struct pollfd (&)[MAX_EVENTS], int&);
    int               handleNewConnection(int, struct pollfd (&)[MAX_EVENTS], int&,
                                          std::map<int, struct ClientContext>&);
    void              handleRead(int, int, struct pollfd (&)[MAX_EVENTS], int&, ContextMap&);
    int               handleResponses(int, int, struct pollfd (&)[MAX_EVENTS], int&, ContextMap&);
    void              handleClientHangup(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context);
    void              handlePartialRequest(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context);
};
