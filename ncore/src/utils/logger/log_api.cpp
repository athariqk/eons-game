#include <utils/logger/log_channel.h>
#include <utils/logger/logger.h>

namespace nc::log {

void log_message( const char* channel, int level, const char* file, const char* func, int line, const char* message )
{
    if (static_cast<Level>( level ) < static_cast<Level>( g_min_level.load() ))
        return;
    auto lvl = static_cast<log::Level>( level );
    log::SourceLoc loc{ file, func, line };
    log::Logger::get_instance().channel( channel )->write( lvl, loc, "{}", message );
}

} // namespace nc::log
