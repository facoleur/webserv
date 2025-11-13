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

void Server::disconnect_client(int& index, int& client_fd, struct pollfd* pfds, int& nfds,
                               std::map<int, ClientContext>& context) {

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
std::vector<int>& Server::initListenerSockets(struct pollfd pfds[MAX_EVENTS], int& nfds) {
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
            setPollFd((*pfds)[nfds], listener, POLLIN, 0);
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
int Server::handleNewConnection(int listener, struct pollfd* pfds[], int& nfds,
                                std::map<int, struct ClientContext>& context) {
    int new_client_fd = accept(listener, NULL, NULL);
    if (new_client_fd < 0) {
        DEBUG_LOG("accept error");
        return -1;
    }
    fcntl(new_client_fd, F_SETFL, O_NONBLOCK);
    setPollFd((*pfds)[nfds], new_client_fd, POLLIN, 0);
    context[new_client_fd]              = ClientContext();
    context[new_client_fd].server_index = _listenerToServerIdx[listener];
    nfds++;
    DEBUG_LOG("new client connected on server index " + to_string(context[new_client_fd].server_index));
    return 0;
}

void Server::run() {
    struct pollfd                       pfds[MAX_EVENTS];
    int                                 nfds = 0;
    std::map<int, struct ClientContext> context;
    std::vector<int>                    listen_fds;

    listen_fds = initListenerSockets(pfds, nfds);
    if (listen_fds.empty()) {
        std::cerr << "error getting listening server sockets" << std::endl;
        exit(1); // yes ?
    }

    while (1) {
        DEBUG_LOG("\n***** while loop start *****\n\t- nfds: " + to_string(nfds) +
                  "\n\t- pfds[nfds].events: " + to_string(pfds[nfds].events));
        if (context[nfds].write_buffer.empty())
            DEBUG_LOG("\t- write buffer: empty");
        else
            DEBUG_LOG("\t- write buffer: not empty");
        DEBUG_LOG("\n(((((POLL)))))");
        if (poll(pfds, nfds, TIMEOUT) < 0) {
            DEBUG_LOG("poll error");
            continue;
        }

        // handle events of each pollfd
        for (int i = 0; i < nfds; i++) {
            DEBUG_LOG("* for loop: i == " + to_string(i) + " *");
            int listener = pfds[i].fd;

            if (pfds[i].revents & (POLLERR | POLLNVAL)) {
                DEBUG_LOG("disconnect 1: poll() returned POLLERR or POLLNVAL");
                disconnect_client(i, listener, pfds, nfds, context);
                continue;
            }

            if (pfds[i].revents & (POLLIN | POLLHUP))
                DEBUG_LOG("POLLIN & POLLHUP detected");

            // Accept on any listening socket
            if (pfds[i].revents & POLLIN) {
                if (_listenerToServerIdx.count(listener)) {
                    handleNewConnection(listener, &pfds, nfds, context);
                    continue;
                }
            }
            DEBUG_LOG("passed: if (_listenerToServerIdx.count(listener) && (pfds[i].revents & POLLIN))");

            if (pfds[i].revents & POLLIN) { /* Reading */
                DEBUG_LOG("POLLIN revents");
                char           tmp[READ_SIZE + 1];
                int            len = read(listener, tmp, READ_SIZE);
                ClientContext& ctx = context[listener];
                if (len == 0) { // client closed their send side
                    if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL) {
                        add_bad_request_to_queue(ctx); // the request was partial and not in the queue
                        DEBUG_LOG("handle_requests: len == 0, partial request. added bad request to queue");
                    }
                    handle_requests(context[listener], pfds[i]);
                    DEBUG_LOG("disconnect 2: read returned 0");
                    disconnect_client(i, listener, pfds, nfds, context);
                    continue;
                }
                if (len < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) { // EAGAIN: No data is available to read,
                        DEBUG_LOG("EAGAIN - no more data");        //  or a write would block
                        continue;
                    }
                    DEBUG_LOG("disconnect 3: read error"); // Real error
                    disconnect_client(i, listener, pfds, nfds, context);
                    continue;
                }
                /* Parsing */
                tmp[len] = '\0';
                ctx.req_parser.feed(tmp, ctx.requests);
                if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL) { /* Need to parse more */
                    DEBUG_LOG("Req partial");
                    continue;
                }
                handle_requests(ctx, pfds[i]);
                if (!ctx.write_buffer.empty()) { // if we have something to write back to the client,
                    pfds[i].events |= POLLOUT;   // add POLLOUT to watched events for the next poll()
                                                 // reset revents to 0 ?
                    DEBUG_LOG("events set to |= POLLOUT");
                } else
                    DEBUG_LOG("POLLOUT not added to events");
                DEBUG_LOG("\"if (pfds[i].revents & POLLIN)\": continuing");
                continue;
            }
            DEBUG_LOG("passed: if revents & POLLIN");

            /* Response handling */
            if (pfds[i].revents & POLLOUT) {
                DEBUG_LOG("POLLOUT revents");
                std::string& buf = context[listener].write_buffer;
                while (!buf.empty()) {
                    ssize_t sent = write(listener, buf.data(), buf.size());
                    DEBUG_LOG("written bytes: " + to_string(sent));
                    if (sent > 0) {
                        buf.erase(0, sent);
                        continue;
                    }
                    if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        break; // socket buffer full, wait for next POLLOUT
                    }
                    // error or connection closed
                    DEBUG_LOG("disconnect 4: error or connection closed while sending back response");
                    disconnect_client(i, listener, pfds, nfds, context);
                    break;
                }
                if (buf.empty()) {              // why this condition ?
                    pfds[i].events &= ~POLLOUT; // Stop watching for POLLOUT. ~ : bitwise NOT
                    // ??? pfds[i].events = POLLIN; ??? // reset POLLIN ?
                }
                continue;
            }

            if (pfds[i].revents & POLLHUP) { // POLLHUP – The device or socket has been disconnected.  This flag is
                                             // output only, and ignored if present in the input events bitmask
                                             // Note that POLLHUP and POLLOUT are mutually exclusive and should never
                                             // be present in the revents bitmask at the same time.
                DEBUG_LOG("disconnect 5 : POLLHUP");
                disconnect_client(i, listener, pfds, nfds, context);
                continue;
            }
            DEBUG_LOG("passed: if revents & POLLOUT");

            DEBUG_LOG("passed: if revents & POLLHUP");
            DEBUG_LOG("*** end of for loop ***");
        }
        DEBUG_LOG("\n***** end of while loop *****");
    }
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
