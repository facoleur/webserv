// Server.cpp

#include "Server.hpp"
#include "Response.hpp"
#include "RequestParser.hpp"

std::ostream& operator<<(std::ostream& os, struct pollfd pfd) {
    os << "fd: " << pfd.fd << std::endl;
    os << "events: " << pfd.events << std::endl;
    os << "revents: " << pfd.revents << std::endl;
    return os;
}

void Server::disconnect_client(int& index, int& client_fd, struct pollfd* pfds, int& nfds) {
    std::cout << "client disconnected" << std::endl;
    pfds[index] = pfds[nfds];
    index--;
    close(client_fd);
    nfds--;
}

Server::Server() {
}

void Server::new_connection() {
}

void Server::existing_connection() {
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
        if (n < 0) {
            std::cout << "poll err" << std::endl;
            continue;
        }

        for (int i = 0; i < nfds; i++) {
            int cfd = pfds[i].fd;

            if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
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
                char tmp[READ_SIZE + 1];
                int  len = read(cfd, tmp, READ_SIZE);

                if (len <= 0) {
                    disconnect_client(i, cfd, pfds, nfds);
                    continue;
                }

                tmp[len] = '\0';

                context[cfd].req_parser.feed(tmp, context[cfd].requests);

                enum ParserState ps = context[cfd].req_parser.getState();

                if (ps == REQ_PARSE_PARTIAL)
                    continue;

                if (ps == REQ_PARSE_ERROR) {
                    disconnect_client(i, cfd, pfds, nfds);
                    continue;
                }

                handle_requests(context[cfd], cfd);
            }
        }
    }
}

void Server::handle_requests(ClientContext& context, int cfd) {
    std::cout << context.requests.front() << std::endl;

    Response

    std::string Response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 5\r\n\r\nHello";

    write(cfd, Response.c_str(), Response.size());
}

Server::~Server() {
}
