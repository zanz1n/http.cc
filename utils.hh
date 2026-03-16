#include <optional>
#include <stdio.h>
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
  slice();
  slice(const T *data, size_t len);
  ~slice();

  const T *ptr();

  T first();
  T last();
  bool empty();
  size_t length();

  slice<T> at(int start, int end = -1);
  std::basic_string<T> to_string();

  bool operator==(const T *other);
  bool operator==(const slice<T> &other);
};

struct buffer {
private:
  buffer(char *data, int len, bool cpy = 0);

public:
  char *data = NULL;
  size_t len = 0;

  buffer();

  ~buffer();

  void append(const char *buf, int buf_len);

  void append(slice<char> buf);

  void reset();

  slice<char> slice(int i = 0);
};

int stridx(const char *str, const char c);

int strnidx(const char *str, int len, const char c);

int strnidx(slice<char> buf, const char c);

template <typename T> std::vector<slice<T>> strnsplit(slice<T> buf, const T c);

template <typename T>
std::optional<std::pair<slice<T>, slice<T>>> strnsplit2(slice<T> buf,
                                                        const T c);

} // namespace http

#endif
