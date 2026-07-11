#pragma once

#include <ncore.h>

#if defined( _MSC_VER )
#define NC_DEBUGBREAK() __debugbreak()
#elif defined( __GNUC__ ) || defined( __clang__ )
#define NC_DEBUGBREAK() __builtin_trap()
#else
#define NC_DEBUGBREAK() ( void ) 0
#endif

namespace nc::log {
NCORE_API void handle_assert( const char* expr, const char* msg, const char* file, int line );
}

#ifdef NC_DEBUG

#define NC_ASSERT( expr, msg )                                                                                         \
    do {                                                                                                               \
        if (!( expr )) {                                                                                               \
            nc::log::handle_assert( #expr, msg, __FILE__, __LINE__ );                                                  \
            NC_DEBUGBREAK();                                                                                           \
        }                                                                                                              \
    } while (0)

#define NC_ASSERT_RET( expr, msg )                                                                                     \
    do {                                                                                                               \
        if (!( expr )) {                                                                                               \
            nc::log::handle_assert( #expr, msg, __FILE__, __LINE__ );                                                  \
            NC_DEBUGBREAK();                                                                                           \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define NC_ASSERT_RETVAL( expr, ret_val, msg )                                                                         \
    do {                                                                                                               \
        if (!( expr )) {                                                                                               \
            nc::log::handle_assert( #expr, msg, __FILE__, __LINE__ );                                                  \
            NC_DEBUGBREAK();                                                                                           \
            return ret_val;                                                                                            \
        }                                                                                                              \
    } while (0)

#define NC_ASSERT_NULL_MSG( ptr, msg )                                                                                 \
    do {                                                                                                               \
        if (ptr == nullptr) {                                                                                          \
            nc::log::handle_assert( #ptr, msg, __FILE__, __LINE__ );                                                   \
            NC_DEBUGBREAK();                                                                                           \
        }                                                                                                              \
    } while (0)

#define NC_ASSERT_NULL( ptr ) NC_ASSERT_NULL_MSG( ptr, "pointer to object is null" )

#else

#define NC_ASSERT( expr, msg )                                                                                         \
    do {                                                                                                               \
        if (!( expr )) {                                                                                               \
            nc::log::handle_assert( #expr, msg, __FILE__, __LINE__ );                                                  \
            std::abort();                                                                                              \
        }                                                                                                              \
    } while (0)

#define NC_ASSERT_RET( expr, msg )                                                                                     \
    do {                                                                                                               \
        if (!( expr )) {                                                                                               \
            nc::log::handle_assert( #expr, msg, __FILE__, __LINE__ );                                                  \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define NC_ASSERT_RETVAL( expr, ret_val, msg )                                                                         \
    do {                                                                                                               \
        if (!( expr )) {                                                                                               \
            nc::log::handle_assert( #expr, msg, __FILE__, __LINE__ );                                                  \
            return ret_val;                                                                                            \
        }                                                                                                              \
    } while (0)

#define NC_ASSERT_NULL( ptr )                                                                                          \
    do {                                                                                                               \
        if (ptr == nullptr) {                                                                                          \
            nc::log::handle_assert( #ptr, "pointer is null reference", __FILE__, __LINE__ );                           \
            std::abort();                                                                                              \
        }                                                                                                              \
    } while (0)

#endif
