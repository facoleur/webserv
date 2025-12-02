// Server.cpp

#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Config.hpp"
#include "Enums.hpp"
#include "Logger.hpp"
#include "RequestParser.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

void Server::disconnect_client(int& index, int& client_fd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds,
                               ContextMap& contextMap) {

    contextMap.erase(client_fd);
    pfds[index] = pfds[nfds - 1];
    index--;
    close(client_fd);
    nfds--;
    LOG_INFO("Client " + toString(client_fd) + " disconnected")
}

Server::Server() {
}

Server::Server(const Config& cfg) : _config(cfg) {
}

Server::~Server() {
}

ClientContext::ClientContext(void) : close_after_responses(false), selectedServer(-1) {
}

void Server::setPollFd(struct pollfd& pfd, int socketFd, short events, short revents) {
    pfd.fd      = socketFd;
    pfd.events  = events;
    pfd.revents = revents;
}

// Create one listening socket per server:port
std::vector<int> Server::initListenerSockets(struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {
    std::vector<int>                 listen_fds;
    int                              listener;
    struct sockaddr_in               addr;
    in_addr                          a;
    int                              opt;
    const std::vector<ServerConfig>& servers = _config.getServers();

    std::map<std::pair<std::string, int>, std::vector<int> > listenMap;

    for (size_t si = 0; si < servers.size(); si++) {
        const ServerConfig& srv = servers[si];
        for (size_t pi = 0; pi < srv.listen_ports.size(); pi++) {
            int         port = srv.listen_ports[pi];
            std::string ip   = srv.host.empty() ? "0.0.0.0" : srv.host;

            listenMap[std::make_pair(ip, port)].push_back(si);
        }
    }

    for (std::map<std::pair<std::string, int>, std::vector<int> >::iterator it = listenMap.begin();
         it != listenMap.end(); ++it) {

        const std::string&      ipStr         = it->first.first;
        int                     port          = it->first.second;
        const std::vector<int>& serverIndices = it->second;

        listener = socket(AF_INET, SOCK_STREAM, 0);
        if (listener < 0)
            continue;
        opt = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        fcntl(listener, F_SETFL, O_NONBLOCK);
        fcntl(listener, F_SETFD, FD_CLOEXEC);

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port);

        // ip              = INADDR_ANY;
        // if (!srv.host.empty() && inet_aton(srv.host.c_str(), &a))
        //     ip = a.s_addr;
        // addr.sin_addr.s_addr = ip;

        if (inet_aton(ipStr.c_str(), &a))
            addr.sin_addr = a;
        else
            addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(listener);
            continue;
        }
        if (listen(listener, SOMAXCONN) < 0) {
            close(listener);
            continue;
        }
        setPollFd(pfds[nfds], listener, POLLIN, 0);
        // DEBUG_LOG("setting _listenerToServerIdx[" + toString(listener) + "] to " + toString(si));
        // _listenerToServerIdx[listener] = si;
        _listenerToServers[listener] = serverIndices;
        listen_fds.push_back(listener);
        ++nfds;
        if (nfds >= MAX_EVENTS)
            break;
    }

    return listen_fds;
}

void Server::add_bad_request_to_queue(ClientContext& context) {
    Request req;
    req.setStatusCode(BAD_REQUEST);
    context.requests.push(req);
}

void Server::handle_requests(ClientContext& context, struct pollfd& pfd) {

    RequestRouter router;

    DEBUG_LOG("handle_requests: " + toString(context.requests.size()) + " requests in queue");

    while (!context.requests.empty()) {
        Request& req = context.requests.front();
        DEBUG_LOG("- handling request:");
        LOG_INFO("Request:  " + methodToString(req.getMethod()) + " " + req.getPath());
        // DEBUG_LOG(req);

        std::string hostHeader = req.getHeader(HOST);

        int chosenConfig = -1;

        const std::vector<ServerConfig>& serverConfigs = _config.getServers();

        for (size_t j = 0; j < context.availableServers.size(); j++) {
            int index = context.availableServers[j];
            if (serverConfigs[index].matchServerName(hostHeader)) {
                chosenConfig = index;
                break;
            }
        }

        if (chosenConfig == -1)
            chosenConfig = context.availableServers[0];

        ServerConfig& config = _config.getServers().at(chosenConfig);
        Response      res    = router.route(req, config);

        if (res.isError()) {
            std::string reasonPhrase(ReasonPhrase::get(res.getStatusCode()));
            DEBUG_LOG("handle_requests() exiting with error: " + reasonPhrase);
            context.write_buffer.append(res.serialize());
            std::queue<Request> empty;
            std::swap(context.requests, empty);
            context.close_after_responses = true;
            break;
        }

        context.write_buffer.append(res.serialize());
        context.requests.pop();
    }
    pfd.events = POLLOUT;
}

// handles partial request i.e. unfinished request but no more POLLIN revents (see Server::run() loop)
// this is a case of bad request
void Server::handlePartialRequest(ClientContext& context, struct pollfd& pfd) {
    add_bad_request_to_queue(context); // the request was partial and not in the queue
    DEBUG_LOG("handlePartialRequest: added bad request to queue");
    handle_requests(context, pfd);
    context.req_parser.setState(REQ_PARSE_COMPLETE);
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
    context[new_client_fd] = ClientContext();
    // context[new_client_fd].server_index     = _listenerToServerIdx.at(listener);
    context[new_client_fd].availableServers = _listenerToServers[listener];
    context[new_client_fd].last_activity    = time(NULL);
    nfds++;
    DEBUG_LOG("new client connected on server index " + toString(context[new_client_fd].server_index));
    return 0;
}

void Server::handleRead(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context) {
    char           tmp[READ_SIZE + 1];
    int            len;
    ClientContext& ctx = context[listener];
    // const ServerConfig& serverConfig = _config.getServers().at(ctx.server_index);

    DEBUG_LOG("handleRead()");
    len               = read(listener, tmp, READ_SIZE);
    ctx.last_activity = time(NULL);
    if (len == 0) { // client closed their send side (or POLLHUP ? unclear but it works)
        DEBUG_LOG("read returned 0 (client closed their send side)");
        ctx.close_after_responses = true;
        if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL) {
            handlePartialRequest(context[listener], pfds[i]);
            return;
        }
    } else if (len < 0) {
        if (errno == EAGAIN ||
            errno == EWOULDBLOCK || // EAGAIN/EWOULDBLOCK: No data is available to read, or a write would block
            errno == EINTR) {       // EINTR: read interrupted before any data arrived by the delivery of a signal
            DEBUG_LOG("read returned < 0 and set errno to EAGAIN, EWOULDBLOCK or EINTR; continuing");
            return;
        }
        DEBUG_LOG("disconnect 3: read error");               // Real error
        disconnect_client(i, listener, pfds, nfds, context); // correct ?
        return;
    } else { // Parsing
        tmp[len]           = '\0';
        size_t maxBodySize = 1024 * 1024; // temp value before choosing the correct serv
        // size_t maxBodySize = serverConfig.client_max_body_size;
        ctx.req_parser.feed(tmp, ctx.requests, maxBodySize);
        if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL)
            return;
    }

    handle_requests(ctx, pfds[i]);
}

void Server::sendResponses(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context) {
    DEBUG_LOG("--- POLLOUT ---");
    std::string& buf = context[listener].write_buffer;
    while (!buf.empty()) {
        ssize_t sent = write(listener, buf.data(), buf.size());
        DEBUG_LOG("written bytes: " + toString(sent));
        if (sent > 0) {
            buf.erase(0, sent);
            context[listener].last_activity = time(NULL);
            break;
        }
        if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) // socket buffer full, wait for next POLLOUT
                return;
            else { // error or connection closed: (sent == 0 ?)
                DEBUG_LOG("disconnect 4: error or connection closed while sending back response");
                disconnect_client(i, listener, pfds, nfds, context);
                return;
            }
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

#include <sys/time.h>

void Server::checkTimeouts(ContextMap& contextMap, int& nfds, struct pollfd (&pfds)[MAX_EVENTS]) {
    std::map<int, ClientContext>::iterator it;
    long                                   currTime;

    currTime = time(NULL);
    if (currTime == -1)
        return; // handleTimeError ?
    it = contextMap.begin();
    while (it != contextMap.end()) {
        if ((currTime - it->second.last_activity) > CLIENT_TIMEOUT) {
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
        DEBUG_LOG("** while loop start **");
        DEBUG_LOG("{nfds}: " + toString(nfds) + " - {pfds[nfds].events}: " + toString(pfds[nfds].events));
        DEBUG_LOG("\n((((( POLL )))))");
        if (poll(pfds, nfds, POLL_TIMEOUT) < 0) {
            DEBUG_LOG("poll error");
            continue;
        }

        checkTimeouts(contextMap, nfds, pfds);

        // handle events of each pollfd
        for (int i = 0; i < nfds; i++) {
            DEBUG_LOG("* for loop: i == " + toString(i) + " *");
            int listener = pfds[i].fd;
            DEBUG_LOG("listener: " + toString(listener));

            if (pfds[i].revents & (POLLERR | POLLNVAL)) {
                DEBUG_LOG("--- POLLERR | POLLNVAL ---\n disconnect 1");
                disconnect_client(i, listener, pfds, nfds, contextMap);
                continue;
            }

            if (pfds[i].revents & POLLIN) {
                DEBUG_LOG("--- POLLIN ---");

                // if (_listenerToServerIdx.count(listener)) // Accept on any listening socket
                if (_listenerToServers.count(listener))
                    handleNewConnection(listener, pfds, nfds, contextMap);
                else
                    handleRead(listener, i, pfds, nfds, contextMap); // Read client data
                continue;
            }

            if (pfds[i].revents & POLLOUT) {
                sendResponses(listener, i, pfds, nfds, contextMap);
                continue;
            }
            DEBUG_LOG("* end for loop *\n");
        }
        DEBUG_LOG("** end while loop **\n");
    }
}
