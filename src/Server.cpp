// Server.cpp

#include "Server.hpp"
#include "MockRequest.hpp"
#include "MockResponse.hpp"
#include "RequestParser.hpp"

std::ostream& operator<<(std::ostream& os, struct pollfd pfd) {
    os << "fd: " << pfd.fd << std::endl;
    os << "events: " << pfd.events << std::endl;
    os << "revents: " << pfd.revents << std::endl;
    return os;
}

Server::Server(Config& conf) {
    (void)conf;
}

void Server::disconnect_client(int& index, int& client_fd, struct pollfd* pfds, int& nfds) {
    std::cout << "client disconnected" << std::endl;
    pfds[index] = pfds[nfds];
    index--;
    close(client_fd);
    nfds--;
}

void Server::new_connection() {
}

void Server::existing_connection() {
}

void Server::run() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(server_fd, F_SETFL, O_NONBLOCK);
    fcntl(server_fd, F_SETFD, FD_CLOEXEC);

    Config conf = this->_config;
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
        if (n < 0)
            std::cout << "poll err" << std::endl;

        for (int i = 0; i < nfds; i++) {
            int cfd = pfds[i].fd;

            if (cfd == server_fd && (pfds[i].revents & POLLIN)) {
                int new_client_fd = accept(cfd, NULL, NULL);
                fcntl(new_client_fd, F_SETFL, O_NONBLOCK);

                if (new_client_fd < 0) {
                    std::cout << "client fd error" << std::endl;
                    continue;
                }

                pfds[nfds].fd      = new_client_fd;
                pfds[nfds].events  = POLLIN;
                pfds[nfds].revents = 0;

                nfds++;

                std::cout << "new client connected" << std::endl;
                std::cout << pfds[i] << std::endl;

                continue;
            }

            MockResponse res;
            if (pfds[i].revents & POLLIN) {

                int len = read(cfd, context[cfd].buffer, READ_SIZE);
                if (len <= 0) {
                    disconnect_client(i, cfd, pfds, nfds);
                }

                context[cfd].buffer[len] = '\0';
                // request: can be multiple requests -> find a way to fill multiple request
                context[cfd].req_parser.feed(context[cfd].buffer, context[cfd].requests);

                switch (context[cfd].req_parser.getState()) {
                    case REQ_PARSE_PARTIAL:
                        continue;
                    case REQ_PARSE_COMPLETE:
                        process_request(context[cfd].requests.front());
                        context[cfd].requests.pop();
                        break;
                    case REQ_PARSE_ERROR:
                        disconnect_client(i, cfd, pfds, nfds);
                        break;
                    default:
                        break;
                }

                continue;
            }
            if (POLLOUT && res) {
                send(res);
            }

            if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                disconnect_client(i, cfd, pfds, nfds);
            }
        }
    }
}

void Server::handle_requests(std::queue<Request>& req) {
    std::cout << req.front() << std::endl;
}
