// Server.cpp

#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <map>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
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
    DEBUG_LOG("checkTimeouts()");
    ContextMap::iterator itClient;
    CgiFdMap::iterator   itCgi;
    long                 currTime;

    currTime = time(NULL);
    if (currTime == -1)
        return; // handleTimeError ?
    DEBUG_LOG("TIMESTAMP: " + toString(time(NULL)));

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
            DEBUG_LOG("Client timed out after (in theory) " + toString(CLIENT_TIMEOUT) + " seconds)");
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
            int cgiFd = current->first;
            DEBUG_LOG("CGI timed out after (in theory) " + toString(CGI_TIMEOUT) + " seconds)");
            handleCgiError(cgiFd, pfds, nfds, GATEWAY_TIMEOUT);
        }
    }
}

// Handle incoming connections
int Server::handleNewConnection(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds,
                                ContextMap& context) {
    (void)i;

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
    context[new_client_fd].pfd              = pfds[nfds];
    nfds++;
    DEBUG_LOG("the new client fd is " + toString(new_client_fd) + " with index " + toString(i) +
              " and nfds == " + toString(nfds));
    return 0;
}

int Server::handleRead(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context) {
    char           tmp[READ_SIZE + 1];
    int            len;
    ClientContext& ctx = context[listener];

    DEBUG_LOG("handleRead()");
    len            = read(listener, tmp, READ_SIZE);
    ctx.lastActive = time(NULL);
    if (len == 0) { // client closed their send side (or POLLHUP ? unclear but it works)
        DEBUG_LOG("read on fd " + toString(listener) + " returned 0 (client closed their send side)");
        ctx.close_after_responses = true;
        if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL) {
            handlePartialRequest(context[listener], i, pfds, nfds);
            return -1;
        }
        pfds[i].events &= ~POLLIN;
        pfds[i].revents = 0;
    } else if (len < 0) {
        DEBUG_LOG("disconnect 3: read error");
        disconnect_client(i, listener, pfds, nfds, context);
        return -1;
    } else { // Parsing
        tmp[len]               = '\0';
        ctx.req_parser._config = _config;
        ctx.req_parser.feed(tmp, ctx.requests, context[listener]);
        if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL)
            return -1;
    }

    handle_requests(ctx, i, pfds, nfds);
    return 0;
}

int Server::sendResponses(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context) {
    DEBUG_LOG("sendResponses()");
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
            return -1;
        }
    }
    if (context[listener].close_after_responses) {
        DEBUG_LOG("disconnect 6: sendResponses (client.close_after_responses: true)");
        disconnect_client(i, listener, pfds, nfds, context);
        return -1;
    }
    pfds[i].events = POLLIN;
    return 0;
}

int Server::writePendingBodyToCgi(int writeFd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {
    DEBUG_LOG("writePendingBodyToCgi()");
    CgiPipeInfo& pipe = _cgiFdMap.at(writeFd);
    CgiInfo*     info = pipe.cgiInfo;
    Request*     req  = info->getRequest();

    const std::string& body      = req->getBody();
    int                written   = info->getBytesWritten();
    int                remaining = static_cast<int>(body.size()) - written;

    if (remaining <= 0) {
        cleanUpCgiFd(writeFd, pfds, nfds);
        return 0;
    }

    ssize_t n = write(writeFd, body.data() + written, remaining);
    if (n > 0) {
        info->setBytesWritten(written + static_cast<int>(n));
        info->setLastActive(time(NULL));
        if (info->getBytesWritten() == static_cast<int>(body.size())) {
            cleanUpCgiFd(writeFd, pfds, nfds);
        }
        return 0;
    }

    handleCgiError(writeFd, pfds, nfds, BAD_GATEWAY);
    return -1;
}

// read output from CGI
int Server::readFromCgi(int readFd, int index, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {
    DEBUG_LOG("readFromCgi()");
    CgiPipeInfo& pipe   = _cgiFdMap.at(readFd);
    CgiInfo*     info   = pipe.cgiInfo;
    int          cgiPID = info->getCgiPID();
    Request*     req    = info->getRequest();
    char         buf[CGI_BUFFER_SIZE];
    if (!req)
        return -1;

    ssize_t n = read(readFd, buf, sizeof(buf));
    if (n > 0) { // append chunk
        std::string chunk(buf, n);
        info->appendToOutput(chunk);
        info->setLastActive(time(NULL));
        DEBUG_LOG("read " + toString(n) + " bytes; output is now: \n\n{\n" + info->getOutput() + "\n}\n\n");
        return 0;
    }
    if (n == 0) { // EOF: close stdout pipe
        DEBUG_LOG("read 0 bytes; cleaning up Cgi fds");
        cleanUpCgiFd(readFd, pfds, nfds);
        int writeFd = info->getWriteFd();
        if (writeFd >= 0)
            cleanUpCgiFd(writeFd, pfds, nfds);
        if (waitForCgiTermination(cgiPID, *req) != 0)
            req->setStatusCode(BAD_GATEWAY);
        req->setState(CGI_DONE);

        (void)index;
        DEBUG_LOG("req->getClientFd() == " + toString(req->getClientFd()));
        int clientFdIndex = findPollFdIndexFromFd(req->getClientFd(), pfds, nfds);
        if (clientFdIndex == -1) {
            DEBUG_LOG("findPollFdIndexFromFd() couldn't find the index for the client fd");
            return -1;
        }
        DEBUG_LOG("findPollFdIndexFromFd() found index " + toString(clientFdIndex));
        handle_requests(req->getClientContext(), clientFdIndex, pfds, nfds);
        return 0;
    }

    DEBUG_LOG("read " + toString(n) + " bytes (read error !)");
    handleCgiError(readFd, pfds, nfds, BAD_GATEWAY);
    return -1;
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

        // DEBUG_LOG("** while loop start **\n{nfds}: " + toString(nfds) + "\n((((( POLL )))))");
        if (poll(pfds, nfds, POLL_TIMEOUT) < 0) {
            DEBUG_LOG("poll error");
            continue;
        }

        for (int index = 0; index < nfds; index++) {
            int   fd      = pfds[index].fd;
            short revents = pfds[index].revents;
            // DEBUG_LOG("* for loop*\nindex == " + toString(index) + "\nfd == " + toString(fd));

            if (revents & (POLLERR | POLLNVAL)) {
                DEBUG_LOG("--- POLLERR | POLLNVAL ---\n disconnect 1");
                if (isCgiPipe(fd)) // if fd==PipeFd from a CGI, clean up the CGI and pre-prepare error response
                    handleCgiError(fd, pfds, nfds, BAD_GATEWAY);
                else // otherwise, fd==clientFd that had a problem -> disconnect it
                    disconnect_client(index, fd, pfds, nfds, contextMap);
                continue;
            }

            // handle CGI stdout as readable even when only POLLHUP is set (Linux pipe EOF)
            if (isCGIPipeRole(fd, CGI_STDOUT) && (revents & (POLLIN | POLLHUP))) {
                readFromCgi(fd, index, pfds, nfds);
                continue;
            }

            if (revents & POLLIN) {
                DEBUG_LOG("--- POLLIN ---");
                if (_listenerToServers.count(fd)) // if we're the listener, open a new connection
                    handleNewConnection(fd, index, pfds, nfds, contextMap);
                else
                    handleRead(fd, index, pfds, nfds, contextMap); // otherwise fd==client fd to read data from
                continue;
            }

            if (revents & POLLHUP) {
                if (isCGIPipeRole(fd, CGI_STDIN)) { // CGI closed before consuming input
                    handleCgiError(fd, pfds, nfds, BAD_GATEWAY);
                } else if (!_listenerToServers.count(fd)) { // treat hangup like a read to flush/close client
                    handleRead(fd, index, pfds, nfds, contextMap);
                }
                continue;
            }

            if (revents & POLLOUT) {
                DEBUG_LOG("--- POLLOUT ---");
                if (isCGIPipeRole(fd, CGI_STDIN)) { // if fd==writePipeFd from a CGI, write the request body to it
                    writePendingBodyToCgi(fd, pfds, nfds);
                    continue;
                }
                sendResponses(fd, index, pfds, nfds, contextMap); // otherwise fd==client socketFd -> write response
                continue;
            }
            DEBUG_LOG("* end for loop *\n");
        }
        DEBUG_LOG("** end while loop **\n");
    }
}
