#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "utils.hh"

namespace http {

buffer::buffer(char *data, int len) {
  this->data = data;
  this->len = len;
}

buffer::buffer() {
  data = NULL;
  len = 0;
}

buffer::~buffer() { free(data); }

void buffer::append(const char *buf, int buf_len) {
  if (data == NULL) {
    data = (char *)malloc(buf_len + 1);
  } else {
    data = (char *)realloc(data, len + buf_len + 1);
  }

  if (data == NULL) {
    throw std::runtime_error("malloc failed");
  }
  strncpy(data + len, buf, buf_len);
  len += buf_len;
}

void buffer::append(http::slice<char> buf) {
  return append(buf.ptr(), buf.length());
}

void buffer::reset() {
  free(this->data);
  this->data = NULL;
  this->len = 0;
}

slice<char> buffer::at(int i) { return http::slice(data + i, len - i); }

int stridx(const char *str, const char c) {
  for (int i = 0;; i++) {
    if (str[i] == '\0')
      return -1;
    else if (str[i] == c)
      return i;
  }
}

int strnidx(const char *str, int len, const char c) {
  int idx = -1;
  for (int i = 0; i < len; i++) {
    if (str[i] == c) {
      idx = i;
      break;
    }
  }
  return idx;
}

int strnidx(slice<char> buf, const char c) {
  return strnidx(buf.ptr(), buf.length(), c);
}

void sockaddr_to_string(const sockaddr *sa, char *ip_str, size_t ip_str_len,
                        int *port) {
  if (sa->sa_family == AF_INET) {
    sockaddr_in *ipv4 = (sockaddr_in *)sa;
    inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, ip_str_len);
    *port = ntohs(ipv4->sin_port); // Convert port from network byte order to
                                   // host byte order
  } else if (sa->sa_family == AF_INET6) {
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)sa;
    inet_ntop(AF_INET6, &(ipv6->sin6_addr), ip_str, ip_str_len);
    *port = ntohs(ipv6->sin6_port);
  } else {
    snprintf(ip_str, ip_str_len, "Unknown family");
    *port = 0;
  }
}

} // namespace http
