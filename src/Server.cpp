// Server.cpp

#include "Server.hpp"
#include "RequestParser.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include "utils.hpp"

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
    std::cout << "client disconnected" << std::endl; // moved to after close() in case close fails
}

Server::Server() {
}

Server::~Server() {
}

void Server::new_connection() {
}

void Server::existing_connection() {
}

void send_bad_request(int cfd);

void add_bad_request_to_queue(ClientContext& context) {
    Request req;
    req.setStatusCode(BAD_REQUEST);
    context.requests.push(req);
}

void Server::run() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(server_fd, F_SETFL, O_NONBLOCK);
    fcntl(server_fd, F_SETFD, FD_CLOEXEC);

    // Config conf = this->_config;
    // TODO: handle conf for ports etc

    struct sockaddr_in addr;

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, SOMAXCONN);

    struct pollfd pfds[MAX_EVENTS];

    int nfds = 1;

    pfds[0].fd      = server_fd;
    pfds[0].events  = POLLIN;
    pfds[0].revents = 0;

    std::map<int, struct ClientContext> context;

    while (1) {
        int n = poll(pfds, nfds, TIMEOUT);
        DEBUG_LOG("\n***** poll *****");
        if (n < 0) {
            DEBUG_LOG("poll err");
            continue;
        }

        for (int i = 0; i < nfds; i++) {
            int cfd = pfds[i].fd;
            if (pfds[i].revents & (POLLERR | POLLNVAL)) { //  POLLHUP | => below POLLIN handling
                DEBUG_LOG("disconnect 1");
                disconnect_client(i, cfd, pfds, nfds, context);
                continue;
            }

            if (cfd == server_fd && (pfds[i].revents & POLLIN)) {
                int new_client_fd = accept(cfd, NULL, NULL);
                if (new_client_fd < 0) {
                    DEBUG_LOG("accept error");
                    continue;
                }
                fcntl(new_client_fd, F_SETFL, O_NONBLOCK);
                pfds[nfds].fd          = new_client_fd;
                pfds[nfds].events      = POLLIN;
                pfds[nfds].revents     = 0;
                context[new_client_fd] = ClientContext();
                nfds++;
                DEBUG_LOG("new client connected");
                DEBUG_LOG(pfds[i]);
                continue;
            }
            DEBUG_LOG("cfd == served_fd passed");

            if (pfds[i].revents & POLLIN) {
                DEBUG_LOG("Pollin revents");
                char           tmp[READ_SIZE + 1];
                int            len = read(cfd, tmp, READ_SIZE);
                ClientContext& ctx = context[cfd];
                /* Reading */
                if (len == 0) { // client closed their send side
                    if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL) {
                        add_bad_request_to_queue(ctx); // the request was partial and not in the queue
                        DEBUG_LOG("handle_requests: len == 0, partial request. added bad request to queue");
                    }
                    handle_requests(context[cfd], pfds[i]);
                    DEBUG_LOG("disconnect 1: read returned 0");
                    disconnect_client(i, cfd, pfds, nfds, context);
                    continue;
                }
                if (len < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) { // No more data available right now - this is normal
                        DEBUG_LOG("EAGAIN - no more data");
                        continue;
                    }
                    DEBUG_LOG("disconnect 2: read error"); // Real error
                    disconnect_client(i, cfd, pfds, nfds, context);
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
                if (!ctx.write_buffer.empty()) {
                    pfds[i].events |= POLLOUT; // ??
                }
                DEBUG_LOG("\"if (pfds[i].revents & POLLIN)\": continuing");
                continue;
            }
            DEBUG_LOG("Pollin passed");

            /* Response handling */
            if (pfds[i].revents & POLLOUT) {
                DEBUG_LOG("Pollout revents");
                std::string& buf = context[cfd].write_buffer;
                while (!buf.empty()) {
                    ssize_t sent = write(cfd, buf.data(), buf.size());
                    DEBUG_LOG("written bytes: " + to_string(sent));
                    if (sent > 0) {
                        buf.erase(0, sent);
                        continue;
                    }
                    if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        break; // socket buffer full, wait for next POLLOUT
                    }
                    // error or connection closed
                    disconnect_client(i, cfd, pfds, nfds, context);
                    break;
                }
                if (buf.empty()) {
                    pfds[i].events &= ~POLLOUT; // Stop watching for POLLOUT. ~ : bitwise NOT
                }
                continue;
            }
            DEBUG_LOG("Pollout passed");

            if (pfds[i].revents & POLLHUP) {
                DEBUG_LOG("disconnect 4 : POLLHUP");
                disconnect_client(i, cfd, pfds, nfds, context);
                continue;
            }
            DEBUG_LOG("Pollhup passed");
            DEBUG_LOG("end of for loop");
        }
        DEBUG_LOG("end of while loop");
    }
}

requestValidity Server::handle_requests(ClientContext& context, struct pollfd& pfd) {
    std::string   responseString;
    RequestRouter router;

    // (void)pfd;

    DEBUG_LOG("handle_requests queue size: " + to_string(context.requests.size()));
    while (!context.requests.empty()) {
        Request& req = context.requests.front();

        // requestValidity reqValidity = req.getValidity();

        // if (reqValidity == INVALID_REQUEST) {
        //     DEBUG_LOG("handle_requests() exiting with: INVALID_REQUEST");
        //     DEBUG_LOG(req);
        //     Response res(400);
        //     context.write_buffer.append(res.serialize());
        //     pfd.events |= POLLOUT; // result: pfd.events == POLLIN | POLLOUT
        //     context.requests.pop();
        //     return INVALID_REQUEST;
        // }
        Response res = router.route(req);
        context.write_buffer.append(res.serialize());
        context.requests.pop();
    }
    pfd.events = POLLOUT; // not sure why it must be = and not |= but it works this way
    DEBUG_LOG("handle_requests() exiting");
    return VALID_REQUEST;
}
