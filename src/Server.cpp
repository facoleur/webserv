// Server.cpp

#include "Server.hpp"
#include "MockResponse.hpp"
#include "RequestParser.hpp"
#include <arpa/inet.h>

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

Server::Server() : _cfg(NULL) {}

Server::Server(const Config& cfg) : _cfg(&cfg) {}

/* Server(const Config& cfg) {
	
}*/


void Server::new_connection() {
}

void Server::existing_connection() {
}

void Server::run() {
    struct pollfd pfds[MAX_EVENTS];
    int nfds = 0;

    // Create one listening socket per server:port
    std::vector<int> listen_fds;
    if (_cfg) {
        const std::vector<ServerConfig>& servers = _cfg->getServers();
        for (size_t si = 0; si < servers.size(); ++si) {
            const ServerConfig& srv = servers[si];
            for (size_t pi = 0; pi < srv.listen_ports.size(); ++pi) {
                int port = srv.listen_ports[pi];
                int lfd = socket(AF_INET, SOCK_STREAM, 0);
                if (lfd < 0) continue;
                int opt = 1;
                setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
                fcntl(lfd, F_SETFL, O_NONBLOCK);
                fcntl(lfd, F_SETFD, FD_CLOEXEC);

                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons(port);
                in_addr_t ip = INADDR_ANY;
                if (!srv.host.empty()) {
                    in_addr a; 
                    if (inet_aton(srv.host.c_str(), &a)) ip = a.s_addr;
                }
                addr.sin_addr.s_addr = ip;

                if (bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                    close(lfd);
                    continue;
                }
                if (listen(lfd, SOMAXCONN) < 0) {
                    close(lfd);
                    continue;
                }
                pfds[nfds].fd = lfd;
                pfds[nfds].events = POLLIN;
                pfds[nfds].revents = 0;
                _listenerToServerIdx[lfd] = si;
                listen_fds.push_back(lfd);
                ++nfds;
                if (nfds >= MAX_EVENTS) break;
            }
            if (nfds >= MAX_EVENTS) break;
        }
    } else {
        // Fallback: single listener on 8080 for development
        int lfd = socket(AF_INET, SOCK_STREAM, 0);
        fcntl(lfd, F_SETFL, O_NONBLOCK);
        fcntl(lfd, F_SETFD, FD_CLOEXEC);
        struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET; addr.sin_port = htons(8080); addr.sin_addr.s_addr = INADDR_ANY;
        bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
        listen(lfd, SOMAXCONN);
        pfds[nfds].fd = lfd; pfds[nfds].events = POLLIN; pfds[nfds].revents = 0; ++nfds;
    }

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

            // Accept on any listening socket
            if (_listenerToServerIdx.count(cfd) && (pfds[i].revents & POLLIN)) {
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
                context[new_client_fd].server_index = _listenerToServerIdx[cfd];

                nfds++;

                std::cout << "new client connected on server index " << context[new_client_fd].server_index << std::endl;
                // std::cout << pfds[i] << std::endl;

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

    std::string mockResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 5\r\n\r\nHello";

    write(cfd, mockResponse.c_str(), mockResponse.size());
}

Server::~Server() {
}
