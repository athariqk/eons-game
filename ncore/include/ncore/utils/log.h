#pragma once

#include <format>
#include <string>

#include <ncore.h>

#define NC_CONCAT_IMPL( a, b ) a##b
#define NC_CONCAT( a, b )      NC_CONCAT_IMPL( a, b )

#ifndef NC_LOG_CHANNEL_NAME
#define NC_LOG_CHANNEL_NAME "NCORE"
#endif

namespace ncore::log {

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
inline constexpr const char* EVENTS   = "EV";
inline constexpr const char* GRAPHICS = "GHS";
inline constexpr const char* GUI      = "GUI";
inline constexpr const char* PHYSICS  = "PHYS";
inline constexpr const char* IO       = "IO";

NCORE_API void
log_message( const char* channel, int level, const char* file, const char* func, int line, const char* message );

} // namespace ncore::log

#define NC_LOG_TRACE_C( cat, ... )                                                                                     \
    do {                                                                                                               \
        auto _nc_msg = std::format( __VA_ARGS__ );                                                                     \
        ncore::log::log_message( cat, 0, __FILE__, __func__, __LINE__, _nc_msg.c_str() );                              \
    } while (0)

#define NC_LOG_DEBUG_C( cat, ... )                                                                                     \
    do {                                                                                                               \
        auto _nc_msg = std::format( __VA_ARGS__ );                                                                     \
        ncore::log::log_message( cat, 1, nullptr, nullptr, 0, _nc_msg.c_str() );                                       \
    } while (0)

#define NC_LOG_INFO_C( cat, ... )                                                                                      \
    do {                                                                                                               \
        auto _nc_msg = std::format( __VA_ARGS__ );                                                                     \
        ncore::log::log_message( cat, 2, nullptr, nullptr, 0, _nc_msg.c_str() );                                       \
    } while (0)

#define NC_LOG_WARN_C( cat, ... )                                                                                      \
    do {                                                                                                               \
        auto _nc_msg = std::format( __VA_ARGS__ );                                                                     \
        ncore::log::log_message( cat, 3, nullptr, nullptr, 0, _nc_msg.c_str() );                                       \
    } while (0)

#define NC_LOG_ERROR_C( cat, ... )                                                                                     \
    do {                                                                                                               \
        auto _nc_msg = std::format( __VA_ARGS__ );                                                                     \
        ncore::log::log_message( cat, 4, __FILE__, __func__, __LINE__, _nc_msg.c_str() );                              \
    } while (0)

#define NC_LOG_FATAL_C( cat, ... )                                                                                     \
    do {                                                                                                               \
        auto _nc_msg = std::format( __VA_ARGS__ );                                                                     \
        ncore::log::log_message( cat, 5, __FILE__, __func__, __LINE__, _nc_msg.c_str() );                              \
    } while (0)

#define NC_LOG_TRACE( ... ) NC_LOG_TRACE_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_DEBUG( ... ) NC_LOG_DEBUG_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_INFO( ... )  NC_LOG_INFO_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_WARN( ... )  NC_LOG_WARN_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_ERROR( ... ) NC_LOG_ERROR_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
#define NC_LOG_FATAL( ... ) NC_LOG_FATAL_C( NC_LOG_CHANNEL_NAME, __VA_ARGS__ )
