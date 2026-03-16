#include "headers.hh"
#include "include/hashmap.h"

#include <cstring>
#include <new>
#include <optional>
#include <string>
#include <utility>

namespace http {

struct headers_pair {
  std::string key;
  std::string value;
};

uint64_t headers_hash(const void *item, uint64_t seed0, uint64_t seed1);

int headers_compare(const void *a, const void *b, void *udata);

void headers_elfree(void *item);

headers::headers() {
  data = hashmap_new(sizeof(headers_pair), 0, 0, 0, headers_hash,
                     headers_compare, headers_elfree, NULL);
}

headers::~headers() { hashmap_free(data); }

void headers::set(std::string key, std::string value) {
  key.shrink_to_fit();
  value.shrink_to_fit();

  auto item = new headers_pair();
  if (item == NULL) {
    throw new std::bad_alloc();
  }

  *item = headers_pair{
      .key = key,
      .value = value,
  };

  hashmap_set(this->data, item);
}

std::optional<std::string *> headers::get(const std::string key) {
  headers_pair kitem = {.key = key};
  auto item = (headers_pair *)hashmap_get(this->data, &kitem);
  if (item == NULL) {
    return std::nullopt;
  }

  return &item->value;
}

std::optional<std::string> headers::remove(const std::string key) {
  headers_pair kitem = {.key = key};
  auto item = (const headers_pair *)hashmap_delete(this->data, &kitem);
  if (item == NULL) {
    return std::nullopt;
  }

  return item->value;
}

uint64_t headers_hash(const void *item, uint64_t seed0, uint64_t seed1) {
  auto pair = (const headers_pair *)item;
  return hashmap_sip(pair->key.c_str(), pair->key.size(), seed0, seed1);
}

int headers_compare(const void *a, const void *b, void *udata) {
  auto h1 = (const headers_pair *)a;
  auto h2 = (const headers_pair *)b;

  if (h1->key.length() != h2->key.length() ||
      h1->value.length() != h2->value.length()) {
    return -1;
  }

  if (!strncmp(h1->key.c_str(), h1->key.c_str(), h1->key.length())) {
    return -1;
  } else if (!strncmp(h1->value.c_str(), h1->value.c_str(),
                      h1->value.length())) {
    return -1;
  }

  return 0;
}

void headers_elfree(void *item) {
  auto pair = (headers_pair *)item;
  delete pair;
}

headers_parser::headers_parser() {}

h_parser_result headers_parser::fill(slice<char> buf) {
  int i = strnidx(buf, '\n');
  if (i >= 0) {
    auto line = buf.at(0, i);
    if (line.length() != 0 && (buf.last() == '\n' || buf.last() == '\r')) {
      line = buf.at(0, i - 1);
    }

    data.append(line);

    auto res = consume_buffer();
    if (res == H_PARSER_ERROR) {
      return h_parser_result(res);
    }

    if (res == H_PARSER_END) {
      slice<char> rest;
      if (buf.length() > i + 1) {
        rest = buf.at(i + 1);
      }

      return h_parser_result(res, rest);
    } else {
      data.append(buf.at(i));
      return h_parser_result(res);
    }

  } else {
    data.append(buf);
    return h_parser_result(H_PARSER_CONTINUE);
  }
}

// processes the line and returns a pair(key, value)
//
// null option if an error happens
std::optional<std::pair<std::string, std::string>>
header_process_line(slice<char> line) {
  int i = strnidx(line, ' ');
  if (i > 0) {
    if (line.ptr()[i - 1] != ':') {
      return std::nullopt;
    }

    slice<char> key_s = line.at(0, i - 1);
    std::string key(key_s.ptr(), key_s.length());

    slice<char> value_s = line.at(i + 1);
    std::string value(value_s.ptr(), value_s.length());

    return std::pair(key, value);
  } else {
    return std::nullopt;
  }
}

// 1 for error
h_parser_result_t headers_parser::consume_buffer() {
  auto line = data.slice();
  if (line.empty()) {
    return H_PARSER_END;
  }

  auto pair = header_process_line(line);
  if (!pair.has_value()) {
    return H_PARSER_ERROR;
  }

  data.reset();

  h.set(pair.value().first, pair.value().second);
  return H_PARSER_CONTINUE;
}

h_parser_result::h_parser_result(h_parser_result_t t) {
  this->t = t;
  this->rest = slice<char>();
}

h_parser_result::h_parser_result(h_parser_result_t t, slice<char> rest) {
  this->t = t;
  this->rest = rest;
}

headers headers_parser::build() { return h; }

} // namespace http
