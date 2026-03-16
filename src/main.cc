#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "epoll.hh"

#define SOCK_QUEUE_SIZE 128
#define EPOLL_QUEUE_SIZE 128

in_addr_t mkipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return a << 24 | b << 16 | c << 8 | d << 0;
}

int main(int argc, char const *argv[]) {
  http::epoll_dri driver(INADDR_ANY, 8080);

  int err = driver.run();
  if (err != 0) {
    http::log::error("Failed to run driver");
  }
}
