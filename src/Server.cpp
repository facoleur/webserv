// Server.cpp

#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "CGI.hpp"
#include "Config.hpp"
#include "Enums.hpp"
#include "Logger.hpp"
#include "RequestParser.hpp"
#include "Server.hpp"
#include "Utils.hpp"

Server::Server() {
}

Server::Server(const Config& cfg) : _config(cfg) {
}

Server::~Server() {
}

void Server::clean() {
    LOG_INFO("Stopping server...")
    for (int i = 0; i < nfds; i++) {
        close(pfds[i].fd);
    }
}

ClientContext::ClientContext(void) : closeAfterResponses(false), selectedServer(-1) {
}

// if client disconnects => go through client requests and check each request's CGIinfo.
// If CGIInfo is there (exists), then end it with
//

void Server::checkTimeouts(ContextMap& contextMap, int& nfds, struct pollfd (&pfds)[MAX_EVENTS]) {
    LOG_DEBUG("checkTimeouts()");
    ContextMap::iterator itClient;
    CgiFdMap::iterator   itCgi;
    long                 currTime;

    currTime = time(NULL);
    if (currTime == -1)
        return;
    LOG_DEBUG("TIMESTAMP: " + toString(time(NULL)));

    // loop over all clients
    itClient = contextMap.begin();
    while (itClient != contextMap.end()) {
        ClientContext& client = itClient->second;

        // if client has passed timeout
        if ((currTime - client.lastActive) > CLIENT_TIMEOUT) {
            int j        = 0;
            int clientFd = itClient->first;

            // check that the client_fd is in pfds
            while (j < nfds && pfds[j].fd != clientFd)
                j++;
            if (j == nfds) { // client fd not found in pfds
                ++itClient;
                continue;
            }

            ++itClient; // to avoid issues when client is removed from contextMap in disconnectClient()
            LOG_DEBUG("Client timed out after (in theory) " + toString(CLIENT_TIMEOUT) + " seconds)");

            disconnectClient(j, clientFd, pfds, nfds, contextMap);
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
            LOG_DEBUG("CGI timed out after (in theory) " + toString(CGI_TIMEOUT) + " seconds)");
            handleCgiError(cgiFd, pfds, nfds, GATEWAY_TIMEOUT);
        }
    }
}

void Server::run() {
    std::vector<int> listenFds;
    ContextMap       contextMap;

    nfds      = 0;
    listenFds = initListenerSockets(pfds, nfds);
    if (listenFds.empty()) {
        std::cerr << "error getting listening server sockets" << std::endl;
        return;
    }

    LOG_INFO("Server started")

    while (1) {
        checkTimeouts(contextMap, nfds, pfds);

        // LOG_DEBUG("** while loop start **\n{nfds}: " + toString(nfds) + "\n((((( POLL )))))");
        if (poll(pfds, nfds, POLL_TIMEOUT) < 0) {
            LOG_DEBUG("poll error");
            continue;
        }

        for (int index = 0; index < nfds; index++) {
            int   fd      = pfds[index].fd;
            short revents = pfds[index].revents;
            // LOG_DEBUG("* for loop*\nindex == " + toString(index) + "\nfd == " + toString(fd));

            if (revents & (POLLERR | POLLNVAL)) {
                LOG_DEBUG("--- POLLERR | POLLNVAL ---\n disconnect 1");
                if (isCgiPipe(fd)) // if fd==PipeFd from a CGI, clean up the CGI and pre-prepare error response
                    handleCgiError(fd, pfds, nfds, BAD_GATEWAY);
                else // otherwise, fd==clientFd that had a problem -> disconnect it
                    disconnectClient(index, fd, pfds, nfds, contextMap);
                continue;
            }

            // handle CGI stdout as readable even when only POLLHUP is set (Linux pipe EOF)
            if (isCGIPipeRole(fd, CGI_STDOUT) && (revents & (POLLIN | POLLHUP))) {
                readFromCgi(fd, pfds, nfds);
                continue;
            }

            if (revents & POLLIN) {
                LOG_DEBUG("--- POLLIN ---");
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
                LOG_DEBUG("--- POLLOUT ---");
                if (isCGIPipeRole(fd, CGI_STDIN)) { // if fd==writePipeFd from a CGI, write the request body to it
                    writePendingBodyToCgi(fd, pfds, nfds);
                    continue;
                }
                sendResponses(fd, index, pfds, nfds, contextMap); // otherwise fd==client socketFd -> write response
                continue;
            }
            LOG_DEBUG("* end for loop *\n");
        }
        LOG_DEBUG("** end while loop **\n");
    }
}
