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
#define TIMEOUT 2500
#define READ_SIZE 8000

struct ClientContext {
    RequestParser       req_parser;
    std::queue<Request> requests;
    struct pollfd       pfd;
    std::string         write_buffer;
    size_t              server_index; // which server accepted the client
};

class Server {
  private:
    const Config          _cfg; // not owning pointer
    struct ClientContext  _state;
    std::map<int, size_t> _listenerToServerIdx; // listen fd -> server index
    int                   executeCgi(const ServerConfig& serverConfig, const LocationConfig* locationConfig,
                                     const Request& request, const std::string& scriptPath,
                                     const std::string& interpreter, std::string& responseBody,
                                     std::map<std::string, std::string>& responseHeaders, int& statusCode,
                                     std::string& statusMessage);

  public:
    Server();
    Server(const Config& cfg);
    ~Server();

    Response        process_request(Request&);
    void            new_connection();
    void            existing_connection();
    void            run();
    requestValidity handle_requests(ClientContext&, struct pollfd&);
    void            print_request();
    void            disconnect_client(int&, int&, struct pollfd*, int&, std::map<int, ClientContext>&);
};
