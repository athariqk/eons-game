#include <utils/logger/assert_handler.h>
#include <utils/logger/logger.h>

namespace nc::log {

void handle_assert( const char* expr, const char* msg, const char* file, int line )
{
    if (Level::LFATAL < g_MIN_LOG_LEVEL.load())
        return;
    auto channel = Logger::get_instance().channel();
    channel->write( Level::LFATAL, SourceLoc{ file, nullptr, line }, "**assertion failed**: {}; {}", expr, msg );
    Logger::get_instance().flush_all();
}

} // namespace nc::log
