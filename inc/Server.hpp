// Server.hpp

#pragma once

#include <poll.h>
#include <vector>

#include "CGI.hpp"
#include "Config.hpp"
#include "Enums.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "Utils.hpp"

class Response;

#define MAX_EVENTS 64
#define CLIENT_TIMEOUT 10000
#define POLL_TIMEOUT 4000
#define READ_SIZE 8000

struct ClientContext {
    ClientContext(void);

    RequestParser       req_parser;
    std::queue<Request> requests;
    struct pollfd       pfd;
    std::string         write_buffer;
    bool                close_after_responses; // if bad request in the queue, set this to true
    // size_t              server_index;          // which server accepted the client
    int              selectedServer;
    std::vector<int> availableServers;
    int              lastActive;
};

struct CgiPipeInfo {
    CgiInfo*    cgiInfo;
    CgiPipeRole role;
};

// maps pollfds.fd to CGIs pipeFds, with each having a PipeRole CGI_STDIN or CGI_STDOUT
typedef std::map<int, CgiPipeInfo> CgiFdMap;

typedef std::map<int, struct ClientContext> ContextMap;

class Server {
  private:
    Config                           _config;
    struct ClientContext             _state;
    std::map<int, std::vector<int> > _listenerToServers;
    CgiFdMap                         _cgiFdMap;

  public:
    Server();
    Server(const Config& cfg);
    ~Server();

    void             run();
    void             add_bad_request_to_queue(ClientContext&);
    void             handle_requests(ClientContext&, struct pollfd (&)[MAX_EVENTS], int, int&);
    void             handleInvalidRequest(ClientContext&, Response&);
    void             disconnect_client(int&, int&, struct pollfd (&)[MAX_EVENTS], int&, std::map<int, ClientContext>&);
    void             setPollFd(struct pollfd&, int, short, short);
    void             removePollEntry(int, struct pollfd (&)[MAX_EVENTS], int&);
    std::vector<int> initListenerSockets(struct pollfd (&)[MAX_EVENTS], int&);
    int  handleNewConnection(int, struct pollfd (&)[MAX_EVENTS], int&, std::map<int, struct ClientContext>&);
    void handleRead(int, int, struct pollfd (&)[MAX_EVENTS], int&, ContextMap&);
    void sendResponses(int, int, struct pollfd (&)[MAX_EVENTS], int&, ContextMap&);
    void handlePartialRequest(ClientContext&, struct pollfd&, int&);
    void checkTimeouts(ContextMap&, int&, struct pollfd (&)[MAX_EVENTS]);

    // CGI
    bool isCgiPipe(int) const;
    int  launchCgi(Request&, struct pollfd (&)[MAX_EVENTS], int&);
    int  setupCgiPipes(int (&)[2], int (&)[2]);
    void storeCgiPipeFds(const int[2], const int[2], Request&, struct pollfd (&)[MAX_EVENTS], int&);
    void cleanUpCgiFds(const int, struct pollfd (&)[MAX_EVENTS], int&);
    int  writeToCgi(int (&)[2], int (&)[2], Request&);
    int  readFromCgi(int (&)[2], Request&);
    int  waitForCgiTermination(pid_t, Request&);
};

// CGI fd lookup:
//
// Use whatever lookup fits your existing data structures but make it O(1). Most teams keep a std::map<int, CgiPipe> or
// std::vector<CgiPipe> keyed by the FD. Each entry stores {CGIState*, PipeRole} where PipeRole is an enum like
// CGI_STDIN vs. CGI_STDOUT. When poll() says “fd 37 is writable,” you grab the entry, see it’s the stdin pipe, and push
// more request-body bytes into the child. When it’s readable and tagged CGI_STDOUT, you drain the CGI output instead.
// The “role” is simply “which direction this FD serves”—it’s how you distinguish between feeding stdin and draining
// stdout, because they need different logic.
