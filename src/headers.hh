#include <optional>
#include <string>

#include "hashmap.h"
#include "utils.hh"

#ifndef _H_HEADERS_HH
#define _H_HEADERS_HH 1

namespace http {

struct headers {
private:
  hashmap *data;

public:
  headers();
  ~headers();

  void set(std::string key, std::string value);

  std::optional<std::string *> get(const std::string key);

  std::optional<std::string> remove(const std::string key);
};

enum h_parser_result_t {
  H_PARSER_END,
  H_PARSER_CONTINUE,
  H_PARSER_ERROR,
};

struct h_parser_result {
  h_parser_result_t t;
  slice<char> rest;

  h_parser_result(h_parser_result_t t);
  h_parser_result(h_parser_result_t t, slice<char> rest);
};

struct headers_parser {
private:
  headers h;
  buffer data;

  // 1 for error
  h_parser_result_t consume_buffer();

public:
  headers_parser();

  h_parser_result fill(slice<char> buf);

  headers build();
};

} // namespace http

#endif
