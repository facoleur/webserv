// Server.cpp

#include "Server.hpp"

Server::Server(Config &conf) { (void)conf; }

void Server::run() {
  int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  Config conf = this->conf;
  // TODO: handle conf for ports etc

  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
  addr.sin_addr.s_addr = INADDR_ANY;

  bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
  listen(server_fd, SOMAXCONN);

  int epfd = epoll_create(1);

  struct epoll_event event;

  event.events = EPOLLIN;
  event.data.fd = server_fd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &event);

  struct epoll_event events[MAX_EVENTS];
  char read_buffer[READ_SIZE + 1];
  (void)read_buffer;

  while (1) {
    int n = epoll_wait(epfd, events, 64, -1);
    for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;

      if (fd == server_fd) {
        int client_fd = accept(server_fd, NULL, NULL);
        struct epoll_event client_ev;
        client_ev.events = EPOLLIN | EPOLLRDHUP;
        client_ev.data.fd = client_fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_ev);
      } else {
        if (events[i].events & EPOLLRDHUP) {
          std::cout << "[Server] Client " << fd
                    << " disconnected cleanly (EPOLLRDHUP).\n";
          close(fd);
          epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
          continue;
        }

        // TODO: each client should have its own request -> map request with
        // correct client_fd
        // static std::string request;
        std::map<int, std::string> requests;

        char buf[1024] = {0};
        ssize_t len = read(fd, buf, sizeof(buf) - 1);

        buf[len] = '\0';
        requests[fd] += buf;
        // request += buf;

        if (len == 0) {
          std::cout << "[Server] Client " << fd << " disconnected (EOF).\n";
          close(fd);
          epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
          continue;
        }

        if (len < 0) {
          std::cout << "read error" << std::endl;
          continue;
        }

        // if (request.find("\r\n\r\n")) {

        std::cout << "client fd: " << fd << std::endl;
        std::cout << "req: " << requests[fd] << std::endl;

        std::cout << "Received (" << len << " bytes): " << buf << "\n";
        std::string res = "HTTP/1.1 222 OK\r\n \
                          Content - \
                          Type : text / plain\r\n \
                          Content - \
                          Length : 5\r\n \
                          \r\n Hello";
        write(fd, res.c_str(), res.size());
        requests[fd].clear();
        // }
      }
    }
  }
}

Server::~Server() {}
