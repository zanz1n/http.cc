#include <format>

#ifndef _H_LOGGER_HH
#define _H_LOGGER_HH 1

namespace http {
namespace log {

template <typename... Args>
void debug(std::format_string<Args...> fmt, Args... args);

template <typename... Args>
void info(std::format_string<Args...> fmt, Args... args);

template <typename... Args>
void warn(std::format_string<Args...> fmt, Args... args);

template <typename... Args>
void error(std::format_string<Args...> fmt, Args... args);

struct logger {
private:
  const char *context;

  template <typename... Args>
  void log(const char *level, std::format_string<Args...> fmt, Args... args);

public:
  logger();

  logger(const char *ctx);

  template <typename... Args>
  void debug(std::format_string<Args...> fmt, Args... args);

  template <typename... Args>
  void info(std::format_string<Args...> fmt, Args... args);

  template <typename... Args>
  void warn(std::format_string<Args...> fmt, Args... args);

  template <typename... Args>
  void error(std::format_string<Args...> fmt, Args... args);
};

} // namespace log
} // namespace http

#endif
