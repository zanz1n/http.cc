#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <stdexcept>

#include "epoll.hh"
#include "headers.hh"
#include "logger.hh"
#include "request.hh"
#include "utils.hh"

namespace http {
// 127.0.0.1
const uint32_t localaddr = 127 << 24 | 1;

#define SOCK_QUEUE_SIZE 128
#define EPOLL_QUEUE_SIZE 128
#define EPOLL_RBUF_SIZE 4096

epoll_conn::epoll_conn(int fd, sockaddr addr, socklen_t addr_len) {
  this->fd = fd;
  this->addr = addr;
  this->addr_len = addr_len;
}

epoll_conn::~epoll_conn() = default;

void epoll_conn::make_progress(slice<char> buf) {
  return parser.make_progress(buf);
}

/**
 * Throws: [std::runtime_error] in case socket setup fails
 */
epoll_dri::epoll_dri(in_addr_t addr, uint16_t port) {
  this->addr.sin_family = AF_INET;
  this->addr.sin_port = htons(port);
  this->addr.sin_addr.s_addr = addr;

  int err;
  if ((err = setup_sock()) != 0) {
    throw std::runtime_error(std::format("setup_sock() -> {}", errno));
  }

  if ((err = setup_epoll()) != 0) {
    throw std::runtime_error(std::format("setup_epoll() -> {}", errno));
  }
}

epoll_dri::~epoll_dri() = default;

void epoll_dri::handle_accept() {
  sockaddr in_addr;
  socklen_t in_addr_len;

  // accept4 avoids a fcntl() syscall to set SOCK_NONBLOCK flag
  // permit check for EWOULDBLOCK/EAGAIN
  int in_fd = accept4(sock_fd, &in_addr, &in_addr_len, SOCK_NONBLOCK);
  if (in_fd < 0) {
    throw std::runtime_error(
        std::format("Accept incomming conn failed: {}", errno));
  }

  epoll_conn *in_meta;
  if ((in_meta = conns[in_fd]) != NULL) {
    delete in_meta;
    conns.erase(in_fd);
    throw std::runtime_error(
        std::format("Duplicate fd[{}] in incomming connection", in_fd));
  }

  in_meta = new epoll_conn(in_fd, in_addr, in_addr_len);
  conns[in_fd] = in_meta;

  char in_ip_str[128];
  int in_port;
  sockaddr_to_string(&in_addr, in_ip_str, 128, &in_port);

  logger.info("Incomming connection[{}] from[{}:{}]", in_fd,
              (const char *)in_ip_str, in_port);

  auto event = epoll_event{
      .events = EPOLLIN,
      .data = {.fd = in_fd},
  };
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, in_fd, &event) < 0) {
    throw std::runtime_error(std::format("epoll_ctl() -> {}", errno));
  }
}

void epoll_dri::handle_read(epoll_conn *c) {
  for (;;) {
    char buf[EPOLL_RBUF_SIZE];
    ssize_t n = read(c->fd, buf, EPOLL_RBUF_SIZE);
    if (n < 0) {
      if (errno == EAGAIN) {
        return;
      }

      throw std::runtime_error(
          std::format("Failed to read from sock: {}", errno));
    }

    c->make_progress(slice<char>(buf, n));
  }
}

void epoll_dri::handle_event(epoll_event event) {
  if (event.events & (EPOLLHUP | EPOLLERR)) {
    terminate(event.data.fd);
    return;
  }

  if (event.data.fd == sock_fd) {
    return handle_accept();
  } else if (event.data.fd == epoll_fd) {
    logger.warn("Unhandled epoll internal event[{}]", event.events);
    return;
  }

  epoll_conn *c = conns[event.data.fd];
  if (c == NULL) {
    logger.warn("Epoll event[{}] with untracked fd[{}]", event.events,
                event.data.fd);
    return;
  }

  if (event.events & (EPOLLIN | EPOLLPRI)) {
    handle_read(c);
  } else if (event.events & EPOLLOUT) {
    logger.debug("EPOLLOUT: {}", event.data.fd);
  } else {
    logger.warn("Unhandled epoll event[{}]", event.events);
  }
}

// zero to OK
int epoll_dri::setup_epoll() {
  if (addr.sin_family != AF_INET) {
    return 1;
  }

  if ((epoll_fd = epoll_create1(EPOLL_CLOEXEC)) < 0) {
    perror("epoll_create1()");
    return 1;
  }

  auto event = epoll_event{
      .events = EPOLLIN,
      .data = {.fd = sock_fd},
  };
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event) < 0) {
    perror("epoll_ctl()");
    return 1;
  }

  logger.info("Epoll[{}] setup", epoll_fd);

  return 0;
}

// zero to OK
int epoll_dri::setup_sock() {
  if (addr.sin_family != AF_INET) {
    return 1;
  }

  auto sock_flags = SOCK_STREAM | SOCK_NONBLOCK;
  if ((sock_fd = socket(addr.sin_family, sock_flags, IPPROTO_IP)) < 0) {
    perror("socket()");
    return 1;
  }

  // assert(sizeof(sockaddr) == sizeof(sockaddr_in));

  int opt = 1;
  auto so_flags = SO_REUSEADDR | SO_REUSEPORT;
  if (setsockopt(sock_fd, SOL_SOCKET, so_flags, &opt, sizeof(int))) {
    perror("setsockopt()");
    return 1;
  }

  if (bind(sock_fd, (sockaddr *)&addr, sizeof(addr))) {
    perror("bind()");
    return 1;
  }

  if (listen(sock_fd, SOCK_QUEUE_SIZE) < 0) {
    perror("listen()");
    return 1;
  }

  logger.info("Socket[{}] setup", sock_fd);

  return 0;
}

// zero to OK
int epoll_dri::run() {
  logger.info("Running on[{}:{}]", addr.sin_addr.s_addr, ntohs(addr.sin_port));

  for (;;) {
    epoll_event evts[EPOLL_QUEUE_SIZE];

    int ready_ct;
    if ((ready_ct = epoll_wait(epoll_fd, evts, EPOLL_QUEUE_SIZE, -1)) < 0) {
      perror("epoll_wait()");
      return 1;
    }

    for (int i = 0; i < ready_ct; i++) {
      try {
        handle_event(evts[i]);
      } catch (std::runtime_error e) {
        logger.error("Failed to handle epoll_event: {}", e.what());
        terminate(evts[i].data.fd);
      }
    }
  }
}

in_addr_t epoll_dri::get_addr() { return this->addr.sin_addr.s_addr; }

in_port_t epoll_dri::get_port() { return this->addr.sin_port; }

// Closes the connection of the given FD (if registred)
void epoll_dri::terminate(int fd) {
  epoll_conn *c;
  if ((c = conns[fd]) != NULL) {
    char in_ip_str[128];
    int in_port;
    sockaddr_to_string(&c->addr, in_ip_str, 128, &in_port);

    logger.info("Connection[{}] from[{}:{}] closed", fd, in_ip_str, in_port);

    close(fd);
    delete c;
    conns.erase(fd);
  } else {
    logger.warn("Termination of connection[{}] failed: not found", fd);
  }
}

} // namespace http
