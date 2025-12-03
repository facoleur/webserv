// Server.cpp

#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <map>
#include <signal.h>
#include <string>
#include <sys/_types/_pid_t.h>
#include <sys/time.h>
#include <utility>
#include <vector>

#include "CGI.hpp"
#include "Config.hpp"
#include "Enums.hpp"
#include "RequestParser.hpp"
#include "RequestRouter.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

Server::Server() {
}

Server::Server(const Config& cfg) : _config(cfg) {
}

Server::~Server() {
}

ClientContext::ClientContext(void) : close_after_responses(false), selectedServer(-1) {
}

void Server::checkTimeouts(ContextMap& contextMap, int& nfds, struct pollfd (&pfds)[MAX_EVENTS]) {
    ContextMap::iterator itClient;
    CgiFdMap::iterator   itCgi;
    long                 currTime;

    currTime = time(NULL);
    if (currTime == -1)
        return; // handleTimeError ?

    // loop over all clients
    itClient = contextMap.begin();
    while (itClient != contextMap.end()) {
        ClientContext& client = itClient->second;

        // if client has passed timeout
        if ((currTime - client.lastActive) > CLIENT_TIMEOUT) {
            int j         = 0;
            int client_fd = itClient->first;

            // check that the client_fd is in pfds
            while (j < nfds && pfds[j].fd != client_fd)
                j++;
            if (j == nfds) { // client fd not found in pfds
                ++itClient;
                continue;
            }
            ++itClient; // to avoid issues when client is removed from contextMap in disconnect_client()

            // close the client_fd, remove itClient from pfds and remove the clientContext from the contextMap
            disconnect_client(j, client_fd, pfds, nfds, contextMap);
            continue; // skip the ++itClient
        } else
            ++itClient;
    }

    // loop over all CGIs
    itCgi = _cgiFdMap.begin();
    while (itCgi != _cgiFdMap.end()) {
        CgiFdMap::iterator current = itCgi++;
        CgiPipeInfo&       pipe    = current->second;

        if (!pipe.cgiInfo)
            continue;

        // CGI has passed timeout
        if ((currTime - pipe.cgiInfo->getLastActive()) > CGI_TIMEOUT) {
            int            cgiFd     = current->first;
            ClientContext* clientPtr = current->second.cgiInfo->getRequest()->getClientPtr();
            handleCgiError(cgiFd, pfds, nfds, clientPtr, GATEWAY_TIMEOUT);
        }
    }
}

// Handle incoming connections
int Server::handleNewConnection(int listener, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context) {
    DEBUG_LOG("handleNewConnection()");
    int new_client_fd = accept(listener, NULL, NULL);
    if (new_client_fd < 0) {
        DEBUG_LOG("accept error: errno is " + toString(errno));
        return -1;
    }
    fcntl(new_client_fd, F_SETFL, O_NONBLOCK);
    setPollFd(pfds[nfds], new_client_fd, POLLIN, 0);
    context[new_client_fd]                  = ClientContext();
    context[new_client_fd].availableServers = _listenerToServers[listener];
    context[new_client_fd].lastActive       = time(NULL);
    nfds++;
    return 0;
}

void Server::handleRead(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context) {
    char           tmp[READ_SIZE + 1];
    int            len;
    ClientContext& ctx = context[listener];

    DEBUG_LOG("handleRead()");
    len            = read(listener, tmp, READ_SIZE);
    ctx.lastActive = time(NULL);
    if (len == 0) { // client closed their send side (or POLLHUP ? unclear but it works)
        DEBUG_LOG("read returned 0 (client closed their send side)");
        ctx.close_after_responses = true;
        if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL) {
            handlePartialRequest(context[listener], i, pfds, nfds);
            return;
        }
    } else if (len < 0) {
        DEBUG_LOG("disconnect 3: read error");
        disconnect_client(i, listener, pfds, nfds, context);
        return;
    } else { // Parsing
        tmp[len] = '\0';
        ctx.req_parser.feed(tmp, ctx.requests, context[listener]);
        if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL)
            return;
    }

    handle_requests(ctx, i, pfds, nfds);
}

void Server::sendResponses(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context) {
    DEBUG_LOG("--- POLLOUT ---");
    std::string& buf = context[listener].write_buffer;
    while (!buf.empty()) {
        ssize_t sent = write(listener, buf.data(), buf.size());
        DEBUG_LOG("written bytes: " + toString(sent));
        if (sent > 0) {
            buf.erase(0, sent);
            context[listener].lastActive = time(NULL);
            break;
        }
        if (sent == -1) { // error or connection closed: (sent == 0 ?)
            DEBUG_LOG("disconnect 4: error or connection closed while sending back response");
            disconnect_client(i, listener, pfds, nfds, context);
            return;
        }
    }
    if (context[listener].close_after_responses) {
        DEBUG_LOG("disconnect 6: sendResponses (client.close_after_responses: true)");
        disconnect_client(i, listener, pfds, nfds, context);
        return;
    }
    pfds[i].events = POLLIN;
    return;
}

void Server::terminateCgiProcess(pid_t pid) {
    if (pid <= 0)
        return;

    kill(pid, SIGTERM);

    for (int attempt = 0; attempt < 5; ++attempt) {
        int   status = 0;
        pid_t ret    = waitpid(pid, &status, WNOHANG);
        if (ret == pid || ret == -1)
            return;
        usleep(50000); // 50 ms grace slice
    }

    kill(pid, SIGKILL);
    waitpid(pid, NULL, WNOHANG);
}

void Server::handleCgiError(const int fd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ClientContext* context,
                            statusCode statusCode) {
    if (!_cgiFdMap[fd].cgiInfo)
        return;

    Request* req    = _cgiFdMap[fd].cgiInfo->getRequest();
    pid_t    cgiPID = _cgiFdMap[fd].cgiInfo->getCgiPID();

    // CGI handling
    cleanUpCgiFds(fd, pfds, nfds);
    terminateCgiProcess(cgiPID);

    // request handling
    if (!context)
        context->close_after_responses = true;
    req->setStatusCode(statusCode);
    req->setState(DONE);
    DEBUG_LOG("handleCgiError(): done");
}

void Server::run() {
    struct pollfd    pfds[MAX_EVENTS];
    int              nfds;
    std::vector<int> listen_fds;
    ContextMap       contextMap;

    nfds       = 0;
    listen_fds = initListenerSockets(pfds, nfds);
    if (listen_fds.empty()) {
        std::cerr << "error getting listening server sockets" << std::endl;
        return;
    }

    while (1) {

        checkTimeouts(contextMap, nfds, pfds);

        DEBUG_LOG("** while loop start **");
        DEBUG_LOG("{nfds}: " + toString(nfds) + " - {pfds[nfds].events}: " + toString(pfds[nfds].events));
        DEBUG_LOG("\n((((( POLL )))))");
        if (poll(pfds, nfds, POLL_TIMEOUT) < 0) {
            DEBUG_LOG("poll error");
            continue;
        }

        // handle events of each pollfd
        for (int i = 0; i < nfds; i++) {
            int listener = pfds[i].fd;
            DEBUG_LOG("* for loop: i == " + toString(i) + " *\n" + "listener: " + toString(listener));

            if (pfds[i].revents & (POLLERR | POLLNVAL)) {
                DEBUG_LOG("--- POLLERR | POLLNVAL ---\n disconnect 1");
                if (isCgiPipe(listener))
                    handleCgiError(listener, pfds, nfds, &contextMap[listener], BAD_GATEWAY);
                else
                    disconnect_client(i, listener, pfds, nfds, contextMap);
                continue;
            }

            // if (isCgiPipe(listener)) {
            //     if (waitForCgiTermination(pid_t, Request&) == -1) {
            //         cleanUpCgiFds(listener, pfds, nfds);
            //         disconnect_client(i, listener, pfds, nfds, contextMap);
            //     }
            //     continue;
            // }

            if (pfds[i].revents & POLLIN) {
                DEBUG_LOG("--- POLLIN ---");

                // if (isCGIReadFD(pfds[i], ....)) // => check CgiFdMap
                // {
                // 		if (readFromCgi(stdoutPipe, request) == -1)
                //     		return -1;
                // }
                // else
                if (_listenerToServers.count(listener))
                    handleNewConnection(listener, pfds, nfds, contextMap);
                else
                    handleRead(listener, i, pfds, nfds, contextMap); // Read client data
                continue;
            }

            if (pfds[i].revents & POLLOUT) {
                // if isCGIWriteFD(pfds[i], ....); => check CgiFdMap
                // {
                // 		if (writeToCgi(stdinPipe, stdoutPipe, request) == -1)
                //     		return -1;
                // }
                // else
                sendResponses(listener, i, pfds, nfds, contextMap);
                continue;
            }
            DEBUG_LOG("* end for loop *\n");
        }
        DEBUG_LOG("** end while loop **\n");
    }
}
