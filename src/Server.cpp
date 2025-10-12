// Server.cpp

#include "Server.hpp"

Server::Server(Config &conf) {}

void Server::run() {
  int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

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

  while (1) {
    int n = epoll_wait(epfd, events, 64, -1);
    for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;

      if (fd == server_fd) {
        // New client connection
        int client_fd = accept(server_fd, NULL, NULL);
        struct epoll_event cev;
        cev.events = EPOLLIN;
        cev.data.fd = client_fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);
      } else {
        // Data ready to read from client
        char buf[1024];
        ssize_t len = read(fd, buf, sizeof(buf));

        std::cout << buf << std::endl;

        if (len <= 0) {
          // Client disconnected or error
          close(fd);
          epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        } else {
          // Basic HTTP response
          char response[] = "HTTP/1.1 200 OK\r\n \
                            Content - \
                            Type : text / plain\r\n \
														Content - \
                            Length : 5\r\n \
														\r\n Hello ";
          write(fd, response, sizeof(response) - 1);
          close(fd);
          epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        }
      }
    }
  }
}

Server::~Server() {}
