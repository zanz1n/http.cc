#include <cstdlib>
#include <cstring>
#include <format>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#include "utils.hh"

namespace http {

template <typename T> slice<T>::slice(const T *data, size_t len) {
  this->data = data;
  this->len = len;
}

template <typename T> slice<T>::~slice() {}

template <typename T> inline const T *slice<T>::ptr() { return this->data; }

template <typename T> inline T slice<T>::first() { return data[0]; }

template <typename T> inline T slice<T>::last() { return data[len - 1]; }

template <typename T> inline bool slice<T>::empty() { return len == 0; }

template <typename T> inline size_t slice<T>::length() { return this->len; }

template <typename T> slice<T> slice<T>::at(int start, int end) {
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

  return slice{
      .data = this->data + start,
      .len = end - start,
  };
}

template <typename T> inline std::basic_string<T> slice<T>::to_string() {
  return std::basic_string<T>(data, len);
}

template <typename T> bool slice<T>::operator==(const T *other) {
  return strncmp(data, (const char *)other, len * sizeof(T)) == 0;
}

template <typename T> bool slice<T>::operator==(const slice<T> &other) {
  if (len != other.len) {
    return false;
  }

  return strncmp((const char *)data, (const char *)other.data,
                 len * sizeof(T)) == 0;
}

buffer::buffer(char *data, int len, bool cpy) {
  this->data = data;
  this->len = len;
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

inline void buffer::append(http::slice<char> buf) {
  return append(buf.ptr(), buf.length());
}

inline void buffer::reset() {
  free(this->data);
  this->data = NULL;
  this->len = 0;
}

inline slice<char> buffer::slice(int i) {
  return http::slice(data + i, len - i);
}

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

} // namespace http
