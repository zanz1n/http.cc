#include "headers.hh"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define SOCK_QUEUE_SIZE 128
#define EPOLL_QUEUE_SIZE 128

in_addr_t mkipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return a << 24 | b << 16 | c << 8 | d << 0;
}

int main(int argc, char const *argv[]) {
  int sock_fd;
  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket(1)");
    return 1;
  }

  int opt = 1;
  const int flags = SO_REUSEADDR | SO_REUSEPORT;
  if (setsockopt(sock_fd, SOL_SOCKET, flags, &opt, sizeof(opt))) {
    perror("setsockopt(1)");
    return 1;
  }

  auto sock_addr = sockaddr_in{
      .sin_family = AF_INET,
      .sin_port = 6969,
      .sin_addr = {.s_addr = mkipv4(127, 0, 0, 1)},
  };
  if (bind(sock_fd, (const sockaddr *)&sock_addr, sizeof(sock_addr))) {
    perror("bind(1)");
    return 1;
  }

  if (listen(sock_fd, SOCK_QUEUE_SIZE)) {
    perror("listen(1)");
    return 1;
  }

  int epoll_fd;
  if ((epoll_fd = epoll_create1(EPOLL_CLOEXEC)) < 0) {
    perror("epoll_create1(1)");
    return 1;
  }

  {
    auto event = epoll_event{
        .events = EPOLLIN,
        .data = {.fd = sock_fd},
    };
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event) < 0) {
      perror("epoll_ctl(1)");
      return 1;
    }
  }

  for (;;) {
    epoll_event evts[EPOLL_QUEUE_SIZE];

    int ready_ct;
    if ((ready_ct = epoll_wait(epoll_fd, evts, EPOLL_QUEUE_SIZE, -1)) < 0) {
      perror("epoll_wait(1)");
      return 1;
    }

    for (int i = 0; i < ready_ct; i++) {
      auto evt = evts[i];

      if (evt.data.fd == sock_fd) {
        // accept
      } else if (evt.data.fd == epoll_fd) {
        // epoll control events
      } else {
        bool closed = false;

        if ((evt.events & EPOLLIN) == EPOLLIN) {
        }
        // new connection
      }
    }
  }
}
