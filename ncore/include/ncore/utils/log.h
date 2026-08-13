#pragma once

#include <format>
#include <functional>

#include <ncore.h>
#include <ncore/core/collection.h>

#define NC_CONCAT_IMPL( a, b ) a##b
#define NC_CONCAT( a, b )      NC_CONCAT_IMPL( a, b )

#ifndef NC_LOG_CHANNEL_NAME
#define NC_LOG_CHANNEL_NAME "NCORE"
#endif

namespace nc::log {

enum class Level : uint8_t {
    LTRACE = 0, // Log level Trace
    LDEBUG,     // Log level Debug
    LINFO,      // Log level Info
    LWARN,      // Log level Warn
    LERROR,     // Log level Error
    LFATAL,     // Log level Fatal
    LOFF        // Log level Off
};

NCAPI StringView level_triplet( Level l );
NCAPI StringView level_name( Level l );
NCAPI StringView level_color( Level l );

struct NCAPI SourceLoc {
    const char* file = nullptr;
    const char* func = nullptr;
    int line         = 0;
    bool empty() const
    {
        return file == nullptr;
    }
};

struct NCAPI LogMsg {
    StringView channel;
    Level level = Level::LOFF;
    SourceLoc loc;
    String payload;
};

static std::atomic<Level> kMIN_LOG_LEVEL = Level::LTRACE;

inline void set_min_level( Level level )
{
    kMIN_LOG_LEVEL.store( level );
}
inline Level get_min_level()
{
    return kMIN_LOG_LEVEL.load();
}

inline void silence()
{
    set_min_level( Level::LOFF );
}
inline void unsilence()
{
    set_min_level( Level::LTRACE );
}

// Engine modules
inline constexpr const char* DEFAULT  = "NCE";
inline constexpr const char* AUDIO    = "AUD";
inline constexpr const char* ECS      = "ECS";
inline constexpr const char* EVENTS   = "EVN";
inline constexpr const char* GRAPHICS = "GPH";
inline constexpr const char* GUI      = "GUI";
inline constexpr const char* PHYSICS  = "PYS";
inline constexpr const char* IO       = "I/O";

NCAPI void log_message( const LogMsg& msg );

/**
 * @brief Print basic info message to default channel and sink (stdout).
 */
NCAPI void print( const String& msg );

/**
 * @brief Print formatted info message to default channel and sink (stdout).
 */
template<typename... Args>
void printf( StringView fmt, Args&&... args )
{
    print( String( std::vformat( fmt, std::make_format_args( args... ) ) ) );
}

using LogMsgCallback = std::function<void( const LogMsg& )>;
using ListenerToken  = std::shared_ptr<LogMsgCallback>;

NCAPI ListenerToken add_listener( const LogMsgCallback& callback );

} // namespace nc::log

//------------------------------------------------------------------------------

#define NC_LOG( p_cat, p_level, p_file, p_func, p_line, ... )                                                          \
    do {                                                                                                               \
        nc::log::LogMsg msg{};                                                                                         \
        msg.channel = p_cat;                                                                                           \
        msg.level   = p_level;                                                                                         \
        msg.loc     = nc::log::SourceLoc{ p_file, p_func, p_line };                                                    \
        msg.payload = std::format( __VA_ARGS__ );                                                                      \
        nc::log::log_message( msg );                                                                                   \
    } while (0)

#define NC_LOG_TRACE_C( cat, ... ) NC_LOG( cat, nc::log::Level::LTRACE, __FILE__, __func__, __LINE__, __VA_ARGS__ )
#define NC_LOG_DEBUG_C( cat, ... ) NC_LOG( cat, nc::log::Level::LDEBUG, nullptr, nullptr, 0, __VA_ARGS__ )
#define NC_LOG_INFO_C( cat, ... )  NC_LOG( cat, nc::log::Level::LINFO, nullptr, nullptr, 0, __VA_ARGS__ )
#define NC_LOG_WARN_C( cat, ... )  NC_LOG( cat, nc::log::Level::LWARN, nullptr, nullptr, 0, __VA_ARGS__ )
#define NC_LOG_ERROR_C( cat, ... ) NC_LOG( cat, nc::log::Level::LERROR, __FILE__, __func__, __LINE__, __VA_ARGS__ )
#define NC_LOG_FATAL_C( cat, ... ) NC_LOG( cat, nc::log::Level::LFATAL, __FILE__, __func__, __LINE__, __VA_ARGS__ )

#define NC_LOG_TRACE( ... ) NC_LOG_TRACE_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_DEBUG( ... ) NC_LOG_DEBUG_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_INFO( ... )  NC_LOG_INFO_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_WARN( ... )  NC_LOG_WARN_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_ERROR( ... ) NC_LOG_ERROR_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_FATAL( ... ) NC_LOG_FATAL_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
