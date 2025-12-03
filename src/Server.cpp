// Server.cpp

#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <sys/time.h>
#include <utility>
#include <vector>

#include "Config.hpp"
#include "Enums.hpp"
#include "RequestParser.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
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
    std::map<int, ClientContext>::iterator it;
    long                                   currTime;

    currTime = time(NULL);
    if (currTime == -1)
        return; // handleTimeError ?
    it = contextMap.begin();
    while (it != contextMap.end()) {
        if ((currTime - it->second.lastActive) > CLIENT_TIMEOUT) {
            int j         = 0;
            int client_fd = it->first;
            while (j < nfds && pfds[j].fd != client_fd)
                j++;
            ++it;
            if (pfds[j].fd == client_fd)
                disconnect_client(j, client_fd, pfds, nfds, contextMap);
            continue;
        }
        ++it;
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
            handlePartialRequest(context[listener], pfds[i], nfds);
            return;
        }
    } else if (len < 0) {
        DEBUG_LOG("disconnect 3: read error");
        disconnect_client(i, listener, pfds, nfds, context);
        return;
    } else { // Parsing
        tmp[len] = '\0';
        ctx.req_parser.feed(tmp, ctx.requests);
        if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL)
            return;
    }

    handle_requests(ctx, pfds, i, nfds);
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

void Server::run() {
    struct pollfd    pfds[MAX_EVENTS];
    int              nfds;
    std::vector<int> listen_fds;
    ContextMap       contextMap;
    CgiFdMap         cgiFdMap;

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
            DEBUG_LOG("* for loop: i == " + toString(i) + " *");
            int listener = pfds[i].fd;
            DEBUG_LOG("listener: " + toString(listener));

            if (pfds[i].revents & (POLLERR | POLLNVAL)) {
                DEBUG_LOG("--- POLLERR | POLLNVAL ---\n disconnect 1");
                if (isCgiPipe(listener))
                    cleanUpCgiFds(listener, pfds, nfds);
                else
                    disconnect_client(i, listener, pfds, nfds, contextMap);
                continue;
            }

            if (isCgiPipe(listener)) {
                if (waitForCgiTermination(pid_t, Request&) == -1) {
                    cleanUpCgiFds(listener, pfds, nfds);
                    disconnect_client(i, listener, pfds, nfds, contextMap);
                }
                continue;
            }

            if (pfds[i].revents & POLLIN) {
                DEBUG_LOG("--- POLLIN ---");

                // if (isCGIReadFD(pfds[i], ....)) // => check CgiFdMap
                //     readFromCGIChild();
                // if (readFromCgi(stdoutPipe, request) == -1)
                //     return -1;
                // else
                if (_listenerToServers.count(listener))
                    handleNewConnection(listener, pfds, nfds, contextMap);
                else
                    handleRead(listener, i, pfds, nfds, contextMap); // Read client data
                continue;
            }

            if (pfds[i].revents & POLLOUT) {
                // if isCGIWriteFD(pfds[i], ....); => check CgiFdMap
                //     writeToCGIChild();
                // if (writeToCgi(stdinPipe, stdoutPipe, request) == -1)
                //     return -1;
                // else
                sendResponses(listener, i, pfds, nfds, contextMap);
                continue;
            }
            DEBUG_LOG("* end for loop *\n");
        }
        DEBUG_LOG("** end while loop **\n");
    }
}
