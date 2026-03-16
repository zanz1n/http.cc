#include <stdio.h>
#include <sys/socket.h>

#include <cstring>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef _H_UTILS_HH
#define _H_UTILS_HH 1

namespace http {

/**
 * Represents a read-only contiguous slice of data with a size.
 *
 * Note that the slice is **NOT** null terminated.
 */
template <typename T> struct slice {
private:
  const T *data = NULL;
  size_t len = 0;

public:
  slice() {
    data = NULL;
    len = 0;
  }

  slice(const T *data, size_t len) {
    this->data = data;
    this->len = len;
  }

  ~slice() = default;

  const T *ptr() { return this->data; }

  T first() { return data[0]; }

  T last() { return data[len - 1]; }

  bool empty() { return len == 0; }

  size_t length() { return this->len; }

  slice<T> at(int start, int end = -1) {
    if (start >= this->len) {
      throw std::out_of_range(
          std::format("slice<{}>::at starting on index {} out of bounds",
                      typeid(T).name(), start));
    }

    if (end > 0) {
      if (start >= end) {
        throw std::out_of_range(
            std::format("slice<{}>::at start >= end", typeid(T).name()));
      }
    } else {
      end = this->len;
    }

    if ((end - start) > this->len) {
      throw std::out_of_range(
          std::format("slice<{}>::at on index {}:{} out of bounds",
                      typeid(T).name(), start, end));
    }

    return slice(this->data + start, end - start);
  }

  std::basic_string<T> to_string() { return std::basic_string<T>(data, len); }

  bool operator==(const T *other) {
    return strncmp(data, (const char *)other, len * sizeof(T)) == 0;
  }

  bool operator==(const slice<T> &other) {
    if (len != other.len) {
      return false;
    }

    return strncmp((const char *)data, (const char *)other.data,
                   len * sizeof(T)) == 0;
  }
};

struct buffer {
private:
  buffer(char *data, int len);

public:
  char *data = NULL;
  size_t len = 0;

  buffer();
  ~buffer();

  void append(const char *buf, int buf_len);

  void append(slice<char> buf);

  void reset();

  slice<char> at(int i = 0);
};

int stridx(const char *str, const char c);

int strnidx(const char *str, int len, const char c);

int strnidx(slice<char> buf, const char c);

template <typename T>
std::vector<slice<T>> strnsplit(slice<char> buf, const T c) {
  std::vector<slice<T>> data;

  const T *ptr = buf.ptr();
  int last_split = 0;
  for (int i = 0; i < buf.length(); i++) {
    if (ptr[i] == c) {
      data.push_back(slice<T>(ptr + last_split, i));
      last_split = i;
    }
  }

  if (last_split != 0) {
    data.push_back(slice<T>(ptr + last_split, buf.length()));
  }

  return data;
}

template <typename T>
std::optional<std::pair<slice<T>, slice<T>>> strnsplit2(slice<T> buf,
                                                        const T c) {
  const T *ptr = buf.ptr();

  int idx = -1;
  for (int i = 0; i < buf.length(); i++) {
    if (ptr[i] == c) {
      idx = i;
      break;
    }
  }

  return std::pair(buf.at(0, idx), buf.at(idx + 1));
}

void sockaddr_to_string(const sockaddr *sa, char *ip_str, size_t ip_str_len,
                        int *port);

} // namespace http

// template <> struct std::formatter<http::slice<char>, char> {
//   constexpr auto parse(auto &ctx) { return ctx.begin(); }
// };

#endif
