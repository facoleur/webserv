// Server.cpp

#include "Server.hpp"
#include "RequestParser.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include <arpa/inet.h>

std::ostream& operator<<(std::ostream& os, struct pollfd pfd) {
    os << "fd: " << pfd.fd << std::endl;
    os << "events: " << pfd.events << std::endl;
    os << "revents: " << pfd.revents << std::endl;
    return os;
}

void Server::disconnect_client(int& index, int& client_fd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds,
                               ContextMap& context) {

    // handle_requests(context[client_fd], client_fd);
    context.erase(client_fd);
    pfds[index] = pfds[nfds - 1];
    index--;
    close(client_fd);
    nfds--;
    std::cout << "client disconnected" << std::endl;
}

Server::Server() : _cfg(NULL) {
}

Server::Server(const Config& cfg) : _cfg(&cfg) {
}

/* Server(const Config& cfg) {

}*/

Server::~Server() {
}

void Server::new_connection() {
}

void Server::existing_connection() {
}

requestValidity Server::handle_requests(ClientContext& context, struct pollfd& pfd) {
    std::string   responseString;
    RequestRouter router;

    (void)pfd;

    DEBUG_LOG("handle_requests queue size: " + to_string(context.requests.size()));
    while (!context.requests.empty()) {
        Request& req = context.requests.front();

        // requestValidity reqValidity = req.getValidity();

        // if (reqValidity == INVALID_REQUEST) {
        //     DEBUG_LOG("handle_requests() exiting with: INVALID_REQUEST");
        //     DEBUG_LOG(req);
        //     Response res(400);
        //     context.write_buffer.append(res.serialize());
        //     context.requests.pop();
        //     return INVALID_REQUEST;
        // }
        Response res = router.route(req);
        context.write_buffer.append(res.serialize());
        context.requests.pop();
    }
    pfd.events = POLLOUT;
    DEBUG_LOG("handle_requests() exiting");
    return VALID_REQUEST;
}

void add_bad_request_to_queue(ClientContext& context) {
    Request req;
    req.setStatusCode(BAD_REQUEST);
    context.requests.push(req);
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
    int                              port;
    struct sockaddr_in               addr;
    in_addr_t                        ip;
    in_addr                          a;
    int                              opt;
    const std::vector<ServerConfig>& servers = _cfg->getServers();

    for (size_t si = 0; si < servers.size(); ++si) {
        const ServerConfig& srv = servers[si];
        for (size_t pi = 0; pi < srv.listen_ports.size(); ++pi) {
            port     = srv.listen_ports[pi];
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
            ip              = INADDR_ANY;
            if (!srv.host.empty() && inet_aton(srv.host.c_str(), &a))
                ip = a.s_addr;
            addr.sin_addr.s_addr = ip;

            if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                close(listener);
                continue;
            }
            if (listen(listener, SOMAXCONN) < 0) {
                close(listener);
                continue;
            }
            setPollFd(pfds[nfds], listener, POLLIN, 0);
            _listenerToServerIdx[listener] = si;
            listen_fds.push_back(listener);
            ++nfds;
            if (nfds >= MAX_EVENTS)
                break;
        }
        if (nfds >= MAX_EVENTS)
            break;
    }
    return listen_fds;
}

// Handle incoming connections
int Server::handleNewConnection(int listener, struct pollfd (&pfds)[MAX_EVENTS], int& nfds,
                                ContextMap& context) {
    int new_client_fd = accept(listener, NULL, NULL);
    if (new_client_fd < 0) {
        DEBUG_LOG("accept error");
        return -1;
    }
    fcntl(new_client_fd, F_SETFL, O_NONBLOCK);
    setPollFd(pfds[nfds], new_client_fd, POLLIN, 0);
    context[new_client_fd]              = ClientContext();
    context[new_client_fd].server_index = _listenerToServerIdx[listener];
    nfds++;
    DEBUG_LOG("new client connected on server index " + to_string(context[new_client_fd].server_index));
    return 0;
}

void Server::handleRead(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context)
{                
    char           tmp[READ_SIZE + 1];
    int            len;
    ClientContext& ctx = context[listener];

    len = read(listener, tmp, READ_SIZE);
    if (len == 0) { // client closed their send side
        if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL) {
            add_bad_request_to_queue(ctx); // the request was partial and not in the queue
            DEBUG_LOG("handle_requests: len == 0, partial request. added bad request to queue");
        }
        handle_requests(context[listener], pfds[i]);
        DEBUG_LOG("disconnect 2: read returned 0");
        disconnect_client(i, listener, pfds, nfds, context);
        return;
    }
    if (len < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { // EAGAIN: No data is available to read,
            DEBUG_LOG("EAGAIN - no more data");        //  or a write would block
            return;
        }
        DEBUG_LOG("disconnect 3: read error"); // Real error
        disconnect_client(i, listener, pfds, nfds, context);
        return;
    }
    /* Parsing */
    tmp[len] = '\0';
    ctx.req_parser.feed(tmp, ctx.requests);
    if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL) { /* Need to parse more */
        DEBUG_LOG("Req partial");
        return;
    }
    handle_requests(ctx, pfds[i]);
    if (!ctx.write_buffer.empty()) { // if we have something to write back to the client,
        pfds[i].events |= POLLOUT;   // add POLLOUT to watched events for the next poll()
                                        // reset revents to 0 ?
        DEBUG_LOG("events set to |= POLLOUT");
    } else
        DEBUG_LOG("POLLOUT not added to events");
}

int Server::handleResponses(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context) {
    std::string& buf = context[listener].write_buffer;
    while (!buf.empty()) {
        ssize_t sent = write(listener, buf.data(), buf.size());
        DEBUG_LOG("written bytes: " + to_string(sent));
        if (sent > 0) {
            buf.erase(0, sent);
            return 0;
        }
        if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return -1; // socket buffer full, wait for next POLLOUT
        }
        // error or connection closed
        DEBUG_LOG("disconnect 4: error or connection closed while sending back response");
        disconnect_client(i, listener, pfds, nfds, context);
        return -1;
    }
    if (buf.empty()) {              // why this condition ?
        pfds[i].events &= ~POLLOUT; // Stop watching for POLLOUT. ~ : bitwise NOT
        // ??? pfds[i].events = POLLIN; ??? // reset POLLIN ?
    }
    return 0;
}

void Server::handleClientHangup(int listener, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, ContextMap& context) {
 // POLLHUP – The device or socket has been disconnected.  This flag is
                                             // output only, and ignored if present in the input events bitmask
                                             // Note that POLLHUP and POLLOUT are mutually exclusive and should never
                                             // be present in the revents bitmask at the same time.
                DEBUG_LOG("disconnect 5 : POLLHUP");
                disconnect_client(i, listener, pfds, nfds, context);
}


void Server::run() {
    struct pollfd                       pfds[MAX_EVENTS];
    int                                 nfds = 0;
    ContextMap context;
    std::vector<int>                    listen_fds;

    listen_fds = initListenerSockets(pfds, nfds);
    if (listen_fds.empty()) {
        std::cerr << "error getting listening server sockets" << std::endl;
        // exit(1); // ?
    }

    while (1) {
        DEBUG_LOG("\n** while loop start **\n\t- nfds: " + to_string(nfds) +
                  "\n\t- pfds[nfds].events: " + to_string(pfds[nfds].events));
        if (context[nfds].write_buffer.empty())
            DEBUG_LOG("\t- write buffer: empty");
        else
            DEBUG_LOG("\t- write buffer: not empty");
        DEBUG_LOG("\n((((( POLL )))))");
        if (poll(pfds, nfds, TIMEOUT) < 0) {
            DEBUG_LOG("poll error");
            continue;
        }

        // handle events of each pollfd
        for (int i = 0; i < nfds; i++) {
            DEBUG_LOG("* for loop: i == " + to_string(i) + " *");
            int listener = pfds[i].fd;

            if (pfds[i].revents & (POLLERR | POLLNVAL)) {
                DEBUG_LOG("POLLERR | POLLNVAL");
                DEBUG_LOG("disconnect 1");
                disconnect_client(i, listener, pfds, nfds, context);
                continue;
            }
            DEBUG_LOG("passed POLLERR | POLLNVAL");

            if (pfds[i].revents & (POLLIN | POLLHUP))
                DEBUG_LOG("POLLIN | POLLHUP");

            if (pfds[i].revents & POLLIN) {
                DEBUG_LOG("POLLIN");
                if (_listenerToServerIdx.count(listener)) { // Accept on any listening socket
                    DEBUG_LOG("(_listenerToServerIdx.count(listener) != 0");
                    handleNewConnection(listener, pfds, nfds, context);
                    continue;
                }
                else {
                    DEBUG_LOG("_listenerToServerIdx.count(listener) == 0");
                    handleRead(listener, i, pfds, nfds, context); // Read client data
                    continue;
                }
            }
            DEBUG_LOG("passed POLLIN");

            // Response handling
            if (pfds[i].revents & POLLOUT) {
                if (handleResponses(listener, i, pfds, nfds, context) == -1)
                    break;
                else
                    continue;
            }
            DEBUG_LOG("passed POLLOUT");

            if (pfds[i].revents & POLLHUP) {
                DEBUG_LOG("POLLHUP");
                handleClientHangup(listener, i, pfds, nfds, context);
                continue;
            }

            DEBUG_LOG("passed POLLHUP");
            DEBUG_LOG("* end for loop *\n");
        }
        DEBUG_LOG("** end while loop **");
    }
}
