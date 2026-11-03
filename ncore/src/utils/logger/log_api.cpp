#include <mutex>

#include <ncore/utils/log.h>
#include <utils/logger/logger.h>

namespace nc::log {

namespace {
std::atomic<Level> g_min_log_level = Level::Trace;
}

void set_min_level( Level p_level )
{
    g_min_log_level.store( p_level );
}
Level get_min_level()
{
    return g_min_log_level.load();
}

StringView level_triplet( Level p_level )
{
    switch (p_level) {
        case Level::Trace:
            return "TRC";
        case Level::Debug:
            return "DBG";
        case Level::Info:
            return "INF";
        case Level::Warn:
            return "WRN";
        case Level::Error:
            return "ERR";
        case Level::Fatal:
            return "FAT";
        default:
            return "OFF";
    }
}

StringView level_name( Level p_level )
{
    switch (p_level) {
        case Level::Trace:
            return "TRACE";
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO";
        case Level::Warn:
            return "WARN";
        case Level::Error:
            return "ERROR";
        case Level::Fatal:
            return "FATAL";
        default:
            return "OFF";
    }
}

StringView level_color( Level p_level )
{
    switch (p_level) {
        case Level::Trace:
            return "\x1b[2;37m";
        case Level::Debug:
            return "\x1b[36m";
        case Level::Info:
            return "\x1b[32m";
        case Level::Warn:
            return "\x1b[33m";
        case Level::Error:
            return "\x1b[31m";
        case Level::Fatal:
            return "\x1b[1;31m";
        default:
            return "";
    }
}

void log_message( const LogMsg& p_msg )
{
    if (p_msg.level < g_min_log_level.load())
        return;

    // Serialize sink writes and listener dispatch. The editor's reimport worker
    // logs from a background thread and must not interleave with the main thread.
    static std::mutex g_log_mutex;
    const std::lock_guard<std::mutex> lock( g_log_mutex );

    log::Logger::get_instance().channel( p_msg.channel )->write( p_msg );
}

NCAPI void print( const String& p_msg )
{
    if (Level::Info < g_min_log_level.load())
        return;
    LogMsg msg;
    msg.channel = NC_LOG_CHANNEL_NAME;
    msg.level   = Level::Info;
    msg.payload = p_msg;
    log_message( msg );
}

NCAPI void print_error( const String& p_msg )
{
    if (Level::Error < g_min_log_level.load())
        return;
    LogMsg msg;
    msg.channel = NC_LOG_CHANNEL_NAME;
    msg.level   = Level::Error;
    msg.payload = p_msg;
    log_message( msg );
}

ListenerToken add_listener( const LogMsgCallback& p_callback )
{
    return log::Logger::get_instance().add_listener( p_callback );
}

} // namespace nc::log
