#include <cstdlib>
#include <stdexcept>

#include "request.hh"

namespace http {

request_parser::request_parser() = default;

request_parser::~request_parser() = default;

void request_parser::make_progress(slice<char> buf) {
  if (state == STATE_REQ_LINE) {
    int i = strnidx(buf.ptr(), buf.length(), '\n');
    if (i > 0) {
      // check for \r\n
      if (i != 0 && buf.ptr()[i - 1] == '\r') {
        i--;
      }

      // not i+1 because line terminator must not be copied
      req_line.append(buf.ptr(), i);

      state = STATE_HEADERS;
      return this->make_progress(buf.at(i + 1));
    } else {
      req_line.append(buf.ptr(), buf.ptr());
    }
  } else if (state == STATE_HEADERS) {
    auto res = headers_p.fill(buf);
    if (res.t == H_PARSER_ERROR) {
      throw std::runtime_error("Invalid header format");
    }
    if (res.t == H_PARSER_END) {
      state = STATE_CP_BODY;
      if (!res.rest.empty()) {
        body.append(res.rest);
      }
    }
  } else if (state == STATE_CP_BODY) {
    body.append(buf);
  }
}

method __request_parse_method(slice<char> method) {
  if (method == "GET") {
    return M_GET;
  } else if (method == "POST") {
    return M_POST;
  } else if (method == "PUT") {
    return M_PUT;
  } else if (method == "DELETE") {
    return M_DELETE;
  } else if (method == "PATCH") {
    return M_PATCH;
  } else if (method == "HEAD") {
    return M_HEAD;
  } else if (method == "OPTIONS") {
    return M_OPTIONS;
  } else if (method == "CONNECT") {
    return M_CONNECT;
  } else if (method == "TRACE") {
    return M_TRACE;
  }

  return M_UNKNOWN;
}

// in HTTP/1.1, the two semver units (after /) are separated by .
//
// in HTTP/2 the one semver unit is 2 (the major)
uint8_t __parse_semver_unit(const char unit) {
  if (unit < 48 || unit > 57) {
    throw std::runtime_error("Invalid request version");
  }

  return unit - 48;
}

struct h_line_result {
  method method;
  uint8_t version_major;
  uint8_t version_minor;
  std::string version_str;
  uri uri;
};

h_line_result __request_parse_line(slice<char> line) {
  auto split = strnsplit(line, ' ');
  if (split.size() != 3) {
    throw std::runtime_error("Invalid request line");
  }

  h_line_result res{
      .version_minor = 0,
  };

  res.method = __request_parse_method(split[0]);
  if (res.method == M_UNKNOWN) {
    throw std::runtime_error("Invalid request method");
  }

  auto uri_s = strnsplit2(split[1], '?');
  if (uri_s.has_value()) {
    res.uri.path = uri_s->first.to_string();
    res.uri.query = uri_s->second.to_string();
  } else {
    res.uri.path = split[1].to_string();
  }

  auto hv_s = strnsplit2(split[2], '/');
  if (!hv_s.has_value() || hv_s->first != "HTTP") {
    throw std::runtime_error("Invalid request version");
  }

  slice<char> hv_sem = hv_s->second;
  size_t hv_sem_l = hv_sem.length();

  if (hv_sem_l == 1) {
    res.version_major = __parse_semver_unit(hv_sem.ptr()[0]);
  } else if (hv_sem_l == 3) {
    if (hv_sem.ptr()[1] != '.') {
      throw std::runtime_error("Invalid request version");
    }
    res.version_major = __parse_semver_unit(hv_sem.ptr()[0]);
    res.version_major = __parse_semver_unit(hv_sem.ptr()[2]);
  } else {
    throw std::runtime_error("Invalid request version");
  }

  res.version_str = split[2].to_string();

  return res;
}

request request_parser::build() {
  auto line =
      __request_parse_line(slice<char>(req_line.c_str(), req_line.length()));

  return request{
      .method = line.method,
      .version_major = line.version_major,
      .version_minor = line.version_minor,
      .uri = line.uri,
      .version = line.version_str,
      .headers = headers_p.build(),
      .body = body,
  };
}

} // namespace http
