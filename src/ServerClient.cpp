// ServerClient.cpp

#include "Server.hpp"
#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "Config.hpp"
#include "Enums.hpp"
#include "Logger.hpp"
#include "RequestParser.hpp"
#include "Server.hpp"
#include "Utils.hpp"

// Handle incoming connections
int Server::handleNewConnection(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds,
                                ContextMap& context) {

    LOG_DEBUG("handleNewConnection()");
    int newClientFd = accept(listener, NULL, NULL);
    if (newClientFd < 0) {
        LOG_DEBUG("accept error: errno is " + toString(errno));
        return -1;
    }
    fcntl(newClientFd, F_SETFL, O_NONBLOCK);
    setPollFd(pfds[nfds], newClientFd, POLLIN, 0);
    context[newClientFd]                  = ClientContext();
    context[newClientFd].availableServers = _listenerToServers[listener];
    context[newClientFd].lastActive       = time(NULL);
    context[newClientFd].pfd              = pfds[nfds];
    nfds++;
    (void)i;
    LOG_DEBUG("the new client fd is " + toString(newClientFd) + " with index " + toString(i) +
              " and nfds == " + toString(nfds));
    return 0;
}

size_t getTmpMaxBodySize(const Config& _config) {
    size_t size = 0;

    std::vector<ServerConfig> servers = _config.getServers();
    for (size_t i = 0; i < servers.size(); i++) {
        if (servers[i].clientMaxBodySize > size) {
            size = servers[i].clientMaxBodySize;
        }
        for (size_t j = 0; j < servers[i].locations.size(); j++) {
            LocationConfig& loc = servers[i].locations[j];
            if (loc.clientMaxBodySize > size) {
                size = loc.clientMaxBodySize;
            }
        }
    }
    return size;
}

// read data from client, call parser to create a request
int Server::handleRead(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context) {
    char           tmp[READ_SIZE + 1];
    int            len;
    ClientContext& ctx = context[listener];

    LOG_DEBUG("handleRead()");
    len            = read(listener, tmp, READ_SIZE);
    ctx.lastActive = time(NULL);
    if (len == 0) { // client closed their send side (or POLLHUP ? unclear but it works)
        LOG_DEBUG("read on fd " + toString(listener) + " returned 0 (client closed their send side)");
        ctx.closeAfterResponses = true;
        if (ctx.reqParser.getState() == REQ_PARSE_PARTIAL) {
            handlePartialRequest(context[listener], i, pfds, nfds);
            return -1;
        }
        pfds[i].events &= ~POLLIN;
        pfds[i].revents = 0;
    } else if (len < 0) {
        LOG_DEBUG("disconnect 3: read error");
        disconnectClient(i, listener, pfds, nfds, context);
        return -1;
    } else { // Parsing
        size_t maxBodySize    = getTmpMaxBodySize(_config);
        tmp[len]              = '\0';
        ctx.reqParser._config = _config;
        ctx.reqParser.feed(tmp, ctx.requests, context[listener], maxBodySize);
        if (ctx.reqParser.getState() == REQ_PARSE_PARTIAL)
            return -1;
    }

    handleRequests(ctx, i, pfds, nfds);
    return 0;
}

// send response(s) to client
int Server::sendResponses(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context) {
    LOG_DEBUG("sendResponses()");
    std::string& buf = context[listener].writeBuffer;
    while (!buf.empty()) {
        ssize_t sent = write(listener, buf.data(), buf.size());
        LOG_DEBUG("written bytes: " + toString(sent));
        if (sent > 0) {
            buf.erase(0, sent);
            context[listener].lastActive = time(NULL);
            break;
        }
        if (sent == -1) { // error or connection closed: (sent == 0 ?)
            LOG_DEBUG("disconnect 4: error or connection closed while sending back response");
            disconnectClient(i, listener, pfds, nfds, context);
            return -1;
        }
    }
    if (context[listener].closeAfterResponses) {
        LOG_DEBUG("disconnect 6: sendResponses (client.closeAfterResponses: true)");
        disconnectClient(i, listener, pfds, nfds, context);
        return -1;
    }
    pfds[i].events = POLLIN;
    return 0;
}
