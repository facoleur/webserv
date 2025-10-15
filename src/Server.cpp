// Server.cpp

#include "Server.hpp"

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
  (void)read_buffer;

  while (1) {
    int n = poll(pfds, nfds, TIMEOUT);

    if (n < 0)
      std::cout << "poll err" << std::endl;

    for (int i = 0; i < nfds; i++) {
      int curr_fd = pfds[i].fd;

      if (curr_fd == server_fd && (pfds[i].revents & POLLIN)) {
        int client_fd = accept(curr_fd, NULL, NULL);
        if (client_fd < 0) {
          std::cout << "client fd error" << std::endl;
          continue;
        }

        pfds[nfds].fd = client_fd;
        pfds[nfds].events = POLLIN;
        pfds[nfds].revents = 0;

        nfds++;

        std::cout << "new client connected" << std::endl;
        std::cout << pfds[i] << std::endl;

        continue;
      }

      if (pfds[i].revents & POLLIN) {
        std::map<int, std::string> requests;
        char buf[10] = {0};
        int len = read(curr_fd, &buf, 10);
        if (len <= 0) {
          disconnect_client(i, curr_fd, pfds, nfds);
        }

        buf[len] = '\0';
        requests[curr_fd] += buf;
        write(curr_fd, requests[curr_fd].c_str(), requests[curr_fd].size());
        std::cout << requests[curr_fd] << std::endl;
        continue;
      }

      if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        disconnect_client(i, curr_fd, pfds, nfds);
      }
    }
  }
}

Server::~Server() {}
