#include "AssertHandler.h"

#include "Logger.h"

namespace ncore::log {

void handle_assert(const char *expr, const char *msg, const char *file, int line) {
    auto channel = Logger::get_instance().get_or_create();
    channel->log(Level::Fatal, SourceLoc{file, nullptr, line}, "**assertion failed: {}** {}", expr, msg);
    Logger::get_instance().flush_all();
}

} // namespace ncore::log
