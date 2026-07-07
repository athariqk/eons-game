#pragma once

#include <format>
#include <string>

#include <ncore.h>

#define NC_CONCAT_IMPL( a, b ) a##b
#define NC_CONCAT( a, b )      NC_CONCAT_IMPL( a, b )

#ifndef NC_LOG_CHANNEL_NAME
#define NC_LOG_CHANNEL_NAME "NCORE"
#endif

namespace nc::log {

static std::atomic<int> g_min_level = 0;

inline void set_min_level( int level )
{
    g_min_level.store( level );
}
inline int get_min_level()
{
    return g_min_level.load();
}

inline void silence()
{
    set_min_level( 6 );
} // OFF
inline void unsilence()
{
    set_min_level( 0 );
}

// Engine modules
inline constexpr const char* DEFAULT  = "NC";
inline constexpr const char* AUDIO    = "AUD";
inline constexpr const char* ECS      = "ECS";
inline constexpr const char* EVENTS   = "EVN";
inline constexpr const char* GRAPHICS = "GPH";
inline constexpr const char* GUI      = "GUI";
inline constexpr const char* PHYSICS  = "PYS";
inline constexpr const char* IO       = "IO";

NCORE_API void
log_message( const char* channel, int level, const char* file, const char* func, int line, const char* message );

} // namespace nc::log

#define NC_LOG( cat, level, file, func, line, ... )                                                                    \
    do {                                                                                                               \
        auto nc_msg = std::format( __VA_ARGS__ );                                                                      \
        nc::log::log_message( cat, level, file, func, line, nc_msg.c_str() );                                          \
    } while (0)

#define NC_LOG_TRACE_C( cat, ... ) NC_LOG( cat, 0, __FILE__, __func__, __LINE__, __VA_ARGS__ )

#define NC_LOG_DEBUG_C( cat, ... ) NC_LOG( cat, 1, nullptr, nullptr, 0, __VA_ARGS__ )

#define NC_LOG_INFO_C( cat, ... ) NC_LOG( cat, 2, nullptr, nullptr, 0, __VA_ARGS__ )

#define NC_LOG_WARN_C( cat, ... ) NC_LOG( cat, 3, nullptr, nullptr, 0, __VA_ARGS__ )

#define NC_LOG_ERROR_C( cat, ... ) NC_LOG( cat, 4, __FILE__, __func__, __LINE__, __VA_ARGS__ )

#define NC_LOG_FATAL_C( cat, ... ) NC_LOG( cat, 5, __FILE__, __func__, __LINE__, __VA_ARGS__ )

#define NC_LOG_TRACE( ... ) NC_LOG_TRACE_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_DEBUG( ... ) NC_LOG_DEBUG_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_INFO( ... )  NC_LOG_INFO_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_WARN( ... )  NC_LOG_WARN_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_ERROR( ... ) NC_LOG_ERROR_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_FATAL( ... ) NC_LOG_FATAL_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
