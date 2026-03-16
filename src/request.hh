#include <cstdint>
#include <optional>
#include <string>

#include "headers.hh"
#include "utils.hh"

#ifndef _H_REQUEST_HH
#define _H_REQUEST_HH 1

namespace http {

enum method {
  M_UNKNOWN,
  M_GET,
  M_POST,
  M_PUT,
  M_DELETE,
  M_PATCH,
  M_HEAD,
  M_OPTIONS,
  M_CONNECT,
  M_TRACE,
};

struct uri {
  std::string path;
  std::optional<std::string> query;
};

struct request {
  method m;
  uint8_t version_major;
  uint8_t version_minor;
  uri u;
  std::string version;
  headers h;
  buffer body;
};

struct request_parser {
public:
  enum {
    STATE_REQ_LINE,
    STATE_HEADERS,
    STATE_CP_BODY,
  } state = STATE_REQ_LINE;

  std::string req_line;
  headers_parser headers_p;
  buffer body;

  // public:
  request_parser();
  ~request_parser();

  void make_progress(slice<char> buf);

  request build();
};

} // namespace http

#endif
