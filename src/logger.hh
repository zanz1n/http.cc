#include <chrono>
#include <format>
#include <unistd.h>

#ifndef _H_LOGGER_HH
#define _H_LOGGER_HH 1

namespace http {
namespace log {

namespace chrono = std::chrono;

template <typename... Args>
void raw_log(const char *ctx, const char *level,
             std::format_string<Args...> fmt, Args... args) {
  auto now_u = chrono::system_clock::now().time_since_epoch();
  uint64_t unix_now = chrono::duration_cast<chrono::seconds>(now_u).count();

  int secs_in_day = unix_now % (3600 * 24);
  int hours = secs_in_day / 3600;
  int mins = (secs_in_day / 60) - (hours * 60);
  int secs = secs_in_day - (mins * 60) - (hours * 3600);

  std::string line = std::format(fmt, std::forward<Args>(args)...);

  std::string out;
  if (ctx != NULL) {
    out = std::format("[{:02}:{:02}:{:02} {}] {}: {}\n", hours, mins, secs,
                      level, ctx, line);
  } else {
    out = std::format("[{:02}:{:02}:{:02} {}] {}\n", hours, mins, secs, level,
                      line);
  }

  write(1, out.c_str(), out.length());
}

struct logger {
private:
  const char *context;

  template <typename... Args>
  void log(const char *level, std::format_string<Args...> fmt, Args... args) {
    raw_log(context, level, fmt, std::forward<Args>(args)...);
  }

public:
  logger();

  logger(const char *ctx);

  template <typename... Args>
  void debug(std::format_string<Args...> fmt, Args... args) {
    log("DEBUG", fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void info(std::format_string<Args...> fmt, Args... args) {
    log(" INFO", fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warn(std::format_string<Args...> fmt, Args... args) {
    log(" WARN", fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void error(std::format_string<Args...> fmt, Args... args) {
    log("ERROR", fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
void debug(std::format_string<Args...> fmt, Args... args) {
  logger().debug(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(std::format_string<Args...> fmt, Args... args) {
  logger().info(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(std::format_string<Args...> fmt, Args... args) {
  logger().warn(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void error(std::format_string<Args...> fmt, Args... args) {
  logger().error(fmt, std::forward<Args>(args)...);
}

} // namespace log
} // namespace http

#endif
