#include <ncore/utils/assert.h>
#include <utils/logger/logger.h>

namespace nc {

void assert_fail( const char* expr, const char* msg, const char* file, int line )
{
    if (log::Level::LFATAL < log::g_MinLogLevel.load())
        return;
    auto channel = log::Logger::get_instance().channel();
    channel->write(
        log::Level::LFATAL, log::SourceLoc{ file, nullptr, line }, "**assertion failed**: {}; {}", expr, msg
    );
    log::Logger::get_instance().flush_all();
}

} // namespace nc
