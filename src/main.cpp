// main.cpp

#include "Server.hpp"

#define MAX_EVENTS 5
#define TIMEOUT 30000
#define READ_SIZE 10

int main(int argc, char const *argv[]) {

  Server server;

  int running = 1;

  int fd = epoll_create(1);

  char read_buffer[READ_SIZE + 1];

  struct epoll_event event;
  struct epoll_event events[MAX_EVENTS];

  event.events = EPOLLIN;
  event.data.fd = 0;

  epoll_ctl(fd, EPOLL_CTL_ADD, 0, &event);

  int event_count;
  while (running) {
    std::cout << "waiting for events..." << std::endl;

    event_count = epoll_wait(fd, events, MAX_EVENTS, 30000);

    std::cout << "event_count: " << event_count << std::endl;

    for (int i = 0; i < event_count; i++) {
      int br = read(events[i].data.fd, read_buffer, READ_SIZE);
      read_buffer[br] = '\0';
      std::cout << "br: " << br << std::endl;
      std::cout << "content: " << read_buffer << std::endl;
    }
  }

  return 0;
}
