#include <arpa/inet.h>
#include <sys/epoll.h>

#include <unordered_map>

#include "logger.hh"
#include "request.hh"
#include "utils.hh"

namespace http {

struct epoll_conn {
  int fd;
  sockaddr addr;
  socklen_t addr_len;

  request_parser parser;

  epoll_conn(int fd, sockaddr addr, socklen_t addr_len);
  ~epoll_conn();

  void make_progress(slice<char> buf);
};

struct epoll_dri {
private:
  sockaddr_in addr;
  int sock_fd;
  int epoll_fd;
  log::logger logger = log::logger("http::epoll");

  std::unordered_map<int, epoll_conn *> conns;

  void handle_accept();
  void handle_read(epoll_conn *c);
  void handle_event(epoll_event event);

  // zero to OK
  int setup_epoll();
  // zero to OK
  int setup_sock();

public:
  /**
   * Throws: [std::runtime_error] in case socket setup fails
   */
  epoll_dri(in_addr_t addr, uint16_t port);
  ~epoll_dri();

  in_addr_t get_addr();
  in_port_t get_port();

  // Closes the connection of the given FD (if registred)
  void terminate(int fd);

  // zero to OK
  int run();
};

} // namespace http
