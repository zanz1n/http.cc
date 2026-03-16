#include "logger.hh"

namespace http {
namespace log {

logger::logger() { context = NULL; }

logger::logger(const char *ctx) { context = ctx; }

} // namespace log
} // namespace http
