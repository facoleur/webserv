// Server.cpp

#include "Server.hpp"
#include "MockResponse.hpp"
#include "RequestParser.hpp"

std::ostream& operator<<(std::ostream& os, struct pollfd pfd) {
    os << "fd: " << pfd.fd << std::endl;
    os << "events: " << pfd.events << std::endl;
    os << "revents: " << pfd.revents << std::endl;
    return os;
}

void Server::disconnect_client(int& index, int& client_fd, struct pollfd* pfds, int& nfds) {
    // handle_requests(context[cfd], cfd);
    pfds[index] = pfds[nfds];
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
        if (n < 0) {
            std::cout << "poll err" << std::endl;
            continue;
        }

        for (int i = 0; i < nfds; i++) {
            int cfd = pfds[i].fd;
            if (pfds[i].revents & (POLLERR | POLLNVAL)) { //  POLLHUP | => below POLLIN handling
                DEBUG_LOG("disconnect 1");
                disconnect_client(i, cfd, pfds, nfds);
                continue;
            }
            if (cfd == server_fd && (pfds[i].revents & POLLIN)) {
                int new_client_fd = accept(cfd, NULL, NULL);
                if (new_client_fd < 0) {
                    std::cout << "accept error" << std::endl;
                    continue;
                }
                fcntl(new_client_fd, F_SETFL, O_NONBLOCK);

                pfds[nfds].fd      = new_client_fd;
                pfds[nfds].events  = POLLIN;
                pfds[nfds].revents = 0;

                context[new_client_fd] = ClientContext();

                nfds++;

                std::cout << "new client connected" << std::endl;
                std::cout << pfds[i] << std::endl;
                continue;
            }

            if (pfds[i].revents & POLLIN) {
                ParserState ps = REQ_PARSE_START;
                char        tmp[READ_SIZE + 1];
                int         len = read(cfd, tmp, READ_SIZE);
                if (len <= 0) {
                    DEBUG_LOG("disconnect 2 : read() returned <= 0");
                    DEBUG_LOG("handle_requests 1");
                    handle_requests(context[cfd], cfd);
                    ps = context[cfd].req_parser.getState();
                    if (context[cfd].req_parser.getState() == REQ_PARSE_PARTIAL)
                        send_bad_request(cfd); // needed because the request was partial and not in the queue
                    disconnect_client(i, cfd, pfds, nfds);
                    continue;
                }

                tmp[len] = '\0';

                context[cfd].req_parser.feed(tmp, context[cfd].requests);
                ps = context[cfd].req_parser.getState();
                if (ps == REQ_PARSE_PARTIAL) {
                    continue;
                }
                DEBUG_LOG("handle_requests 2");
                requestValidity lastRequestValidity =
                    handle_requests(context[cfd], cfd);
                if (lastRequestValidity == INVALID_REQUEST) {
                    DEBUG_LOG("disconnect 3 : invalid request found in the queue");
                    disconnect_client(i, cfd, pfds, nfds);
                    continue;
                }
            }
            if (pfds[i].revents & POLLHUP) {
                DEBUG_LOG("disconnect 4 : POLLHUP");
                disconnect_client(i, cfd, pfds, nfds);
                continue;
            }
        }
    }
}

requestValidity Server::handle_requests(ClientContext& context, int cfd) {
    (void)cfd;
    requestValidity lastRequestValidity;
    std::string     responseString;
    DEBUG_LOG("handle_requests queue size: ");
    DEBUG_LOG(context.requests.size());
    while (!context.requests.empty()) {
        DEBUG_LOG(context.requests.front());
        lastRequestValidity = context.requests.front().getValidity();
        if (lastRequestValidity == INVALID_REQUEST) {
            DEBUG_LOG("handle_requests() exiting with: INVALID_REQUEST");
            context.requests.pop();
            break;
        }
        context.requests.pop();
    }
    if (lastRequestValidity == VALID_REQUEST)
        DEBUG_LOG("handle_requests() exiting with: VALID_REQUEST");

    // while (!context.requests.empty())
    // {
    // 	std::cout << "handle_requests() loop - size: " << context.requests.size() << std::endl;
    // 	if (context.requests.front().getValidity() == INVALID_REQUEST)
    // 	{
    // 		std::cout << "handle_requests(): invalid request found" << std::endl;
    // 		MockResponse resp(400);
    // 		responseString = resp.getResponse();
    // 		write(cfd, responseString.c_str(), responseString.size());
    // 		return false;
    // 	}
    // 	std::cout << "handle_requests(): valid request being handled" << std::endl;
    // 	MockResponse resp(200);
    // 	responseString = resp.getResponse();
    // 	write(cfd, responseString.c_str(), responseString.size());
    // 	context.requests.pop();
    // }
    // close(cfd);
    return lastRequestValidity;
}

void send_bad_request(int cfd) {
    (void)cfd;
    std::cout << "RESPONSE: " << std::endl
              << "---------" << std::endl
              << "HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
}