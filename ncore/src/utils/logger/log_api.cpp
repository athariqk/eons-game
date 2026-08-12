#include <ncore/utils/log.h>
#include <utils/logger/logger.h>

namespace nc::log {

StringView level_triplet( Level l )
{
    switch (l) {
        case Level::LTRACE:
            return "TRC";
        case Level::LDEBUG:
            return "DBG";
        case Level::LINFO:
            return "INF";
        case Level::LWARN:
            return "WRN";
        case Level::LERROR:
            return "ERR";
        case Level::LFATAL:
            return "FAT";
        default:
            return "OFF";
    }
}

StringView level_name( Level l )
{
    switch (l) {
        case Level::LTRACE:
            return "TRACE";
        case Level::LDEBUG:
            return "DEBUG";
        case Level::LINFO:
            return "INFO";
        case Level::LWARN:
            return "WARN";
        case Level::LERROR:
            return "ERROR";
        case Level::LFATAL:
            return "FATAL";
        default:
            return "OFF";
    }
}

StringView level_color( Level l )
{
    switch (l) {
        case Level::LTRACE:
            return "\x1b[2;37m";
        case Level::LDEBUG:
            return "\x1b[36m";
        case Level::LINFO:
            return "\x1b[32m";
        case Level::LWARN:
            return "\x1b[33m";
        case Level::LERROR:
            return "\x1b[31m";
        case Level::LFATAL:
            return "\x1b[1;31m";
        default:
            return "";
    }
}

void log_message( const LogMsg& msg )
{
    if (msg.level < kMIN_LOG_LEVEL.load())
        return;
    log::Logger::get_instance().channel( msg.channel )->write( msg );
}

ListenerToken add_listener( const LogMsgCallback& callback )
{
    return log::Logger::get_instance().add_listener( callback );
}

} // namespace nc::log
