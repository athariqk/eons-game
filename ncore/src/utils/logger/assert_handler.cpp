#include <utils/logger/assert_handler.h>
#include <utils/logger/logger.h>

namespace ncore::log {

void handle_assert(const char* expr, const char* msg, const char* file, int line)
{
    if (Level::Fatal < static_cast<Level>(g_min_level.load()))
        return;
    auto channel = Logger::get_instance().channel();
    channel->write(Level::Fatal, SourceLoc{file, nullptr, line}, "**assertion failed**: {}; {}", expr, msg);
    Logger::get_instance().flush_all();
}

} // namespace ncore::log
