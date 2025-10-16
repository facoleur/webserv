// Server.cpp

#include "Server.hpp"
#include "MockRequest.hpp"
#include "MockResponse.hpp"

std::ostream &operator<<(std::ostream &os, struct pollfd pfd) {
  os << "fd: " << pfd.fd << std::endl;
  os << "events: " << pfd.events << std::endl;
  os << "revents: " << pfd.revents << std::endl;
  return os;
}

Server::Server(Config &conf) { (void)conf; }

void Server::disconnect_client(int &index, int &client_fd, struct pollfd *pfds,
                               int &nfds) {
  std::cout << "client disconnected" << std::endl;
  pfds[index] = pfds[nfds];
  index--;
  close(client_fd);
  nfds--;
}

void Server::run() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  Config conf = this->conf;
  // TODO: handle conf for ports etc

  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
  addr.sin_addr.s_addr = INADDR_ANY;

  bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
  listen(server_fd, SOMAXCONN);

  struct pollfd pfds[MAX_EVENTS];

  int nfds = 1;

  pfds[0].fd = server_fd;
  pfds[0].events = POLLIN;
  pfds[0].revents = 0;

  char read_buffer[READ_SIZE + 1];

  std::map<int, std::string> requests;

  while (1) {
    int n = poll(pfds, nfds, TIMEOUT);

    if (n < 0)
      std::cout << "poll err" << std::endl;

    for (int i = 0; i < nfds; i++) {
      int client_fd = pfds[i].fd;

      if (client_fd == server_fd && (pfds[i].revents & POLLIN)) {
        int new_client_fd = accept(client_fd, NULL, NULL);
        if (new_client_fd < 0) {
          std::cout << "client fd error" << std::endl;
          continue;
        }

        pfds[nfds].fd = new_client_fd;
        pfds[nfds].events = POLLIN;
        pfds[nfds].revents = 0;

        nfds++;

        std::cout << "new client connected" << std::endl;
        std::cout << pfds[i] << std::endl;

        continue;
      }

      if (pfds[i].revents & POLLIN) {
        int len = read(client_fd, &read_buffer, 10);
        if (len <= 0) {
          disconnect_client(i, client_fd, pfds, nfds);
        }

        read_buffer[len] = '\0';
        requests[client_fd] += read_buffer;

        // processing

        if (requests[client_fd].find("0\r\n\r\n") != std::string::npos) {
          // end of request (?)
          MockResponse r = process_request(requests[client_fd]);
          std::string response = r.getResponse();
          write(client_fd, response.c_str(), response.size());

          std::cout << "requests from client " << client_fd << ": "
                    << requests[client_fd] << "[end]" << std::endl;
          requests[client_fd].clear();
        }

        // processing

        continue;
      }

      if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        disconnect_client(i, client_fd, pfds, nfds);
      }
    }
  }
}

MockResponse Server::process_request(std::string &request) {
  (void)request;
  MockResponse res;

  return res;
}

Server::~Server() {}
