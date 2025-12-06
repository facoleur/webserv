// Server.hpp

#pragma once

#include <poll.h>
#include <vector>

#include "CGI.hpp"
#include "Config.hpp"
#include "Enums.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"

class Response;

#define MAX_EVENTS 64
#define CLIENT_TIMEOUT 5
#define POLL_TIMEOUT 0
#define READ_SIZE 8000

struct ClientContext {
    ClientContext(void);

    RequestParser       reqParser;
    std::queue<Request> requests;
    struct pollfd       pfd;
    std::string         writeBuffer;
    bool                closeAfterResponses; // if bad request in the queue, set this to true
    int                 selectedServer;
    std::vector<int>    availableServers;
    int                 lastActive;
};

struct CgiPipeInfo {
    CgiInfo*    cgiInfo;
    CgiPipeRole role;
};

// maps pollfds.fd to CGIs pipeFds, with each having a PipeRole CGI_STDIN or CGI_STDOUT
typedef std::map<int, CgiPipeInfo> CgiFdMap;

// maps a client_fd to a ClientContext (handleNewConnection())
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
    void             addBadRequestToQueue(ClientContext&);
    void             handleRequests(ClientContext&, int, struct pollfd (&pfds)[MAX_EVENTS], int&);
    void             handleInvalidRequest(ClientContext&, Response&, struct pollfd&);
    void             disconnectClient(int&, int&, struct pollfd (&)[MAX_EVENTS], int&, std::map<int, ClientContext>&);
    void             setPollFd(struct pollfd&, int, short, short);
    void             removePollEntry(int, struct pollfd (&)[MAX_EVENTS], int&);
    std::vector<int> initListenerSockets(struct pollfd (&)[MAX_EVENTS], int&);
    int  handleNewConnection(int, int, struct pollfd (&)[MAX_EVENTS], int&, std::map<int, struct ClientContext>&);
    int  handleRead(int, int, struct pollfd (&)[MAX_EVENTS], int&, ContextMap&);
    int  sendResponses(int, int, struct pollfd (&)[MAX_EVENTS], int&, ContextMap&);
    void handlePartialRequest(ClientContext&, int, struct pollfd (&)[MAX_EVENTS], int&);
    void checkTimeouts(ContextMap&, int&, struct pollfd (&)[MAX_EVENTS]);
    int  findPollFdIndexFromFd(int, struct pollfd (&)[MAX_EVENTS], int) const;

    // CGI
    bool isCgiPipe(int) const;
    bool isCGIPipeRole(int, CgiPipeRole) const;
    int  launchCgi(Request&, struct pollfd (&)[MAX_EVENTS], int&);
    int  setupCgiPipes(int (&)[2], int (&)[2]);
    void storeCgiPipeFds(const int[2], const int[2], Request&, struct pollfd (&)[MAX_EVENTS], int&);
    void cleanUpCgiFd(const int, struct pollfd (&)[MAX_EVENTS], int&);
    void cleanUpBothCgiFds(const int, struct pollfd (&)[MAX_EVENTS], int&);
    int  writePendingBodyToCgi(const int, struct pollfd (&)[MAX_EVENTS], int&);
    int  readFromCgi(const int, struct pollfd (&)[MAX_EVENTS], int&);
    int  waitForCgiTermination(pid_t);
    void handleCgiError(const int, struct pollfd (&)[MAX_EVENTS], int&, statusCode);
    void terminateCgiProcess(pid_t);
};
