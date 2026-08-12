#pragma once

#include <ncore.h>

#if defined( _MSC_VER )
#define NC_DEBUGBREAK() __debugbreak()
#elif defined( __GNUC__ ) || defined( __clang__ )
#define NC_DEBUGBREAK() __builtin_trap()
#else
#define NC_DEBUGBREAK() ( void ) 0
#endif

namespace nc {

/**
 * @brief Used to notify than an assertion has failed.
 */
NCAPI void assert_fail( const char* expr, const char* msg, const char* file, int line );

} // namespace nc

#ifdef NC_DEBUG

#define NC_ASSERT( expr, msg )                                                                                         \
    do {                                                                                                               \
        if (!( expr )) {                                                                                               \
            nc::assert_fail( #expr, msg, __FILE__, __LINE__ );                                                         \
            NC_DEBUGBREAK();                                                                                           \
        }                                                                                                              \
    } while (0)

#define NC_FAIL_MSG_RET( expr, msg )                                                                                     \
    do {                                                                                                               \
        if (!( expr )) {                                                                                               \
            nc::assert_fail( #expr, msg, __FILE__, __LINE__ );                                                         \
            NC_DEBUGBREAK();                                                                                           \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define NC_FAIL_MSG_RETVAL( expr, ret_val, msg )                                                                         \
    do {                                                                                                               \
        if (!( expr )) {                                                                                               \
            nc::assert_fail( #expr, msg, __FILE__, __LINE__ );                                                         \
            NC_DEBUGBREAK();                                                                                           \
            return ret_val;                                                                                            \
        }                                                                                                              \
    } while (0)

#define NC_VERIFY_MSG( ptr, msg )                                                                                 \
    do {                                                                                                               \
        if (ptr == nullptr) {                                                                                          \
            nc::assert_fail( #ptr, msg, __FILE__, __LINE__ );                                                          \
            NC_DEBUGBREAK();                                                                                           \
        }                                                                                                              \
    } while (0)

#define NC_VERIFY( ptr ) NC_VERIFY_MSG( ptr, "pointer to object is null" )

#else

#define NC_ASSERT( expr, msg )                                                                                         \
    do {                                                                                                               \
        if (!( expr )) {                                                                                               \
            nc::assert_fail( #expr, msg, __FILE__, __LINE__ );                                                         \
            std::abort();                                                                                              \
        }                                                                                                              \
    } while (0)

#define NC_FAIL_MSG_RET( expr, msg )                                                                                     \
    do {                                                                                                               \
        if (!( expr )) {                                                                                               \
            nc::assert_fail( #expr, msg, __FILE__, __LINE__ );                                                         \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define NC_FAIL_MSG_RETVAL( expr, ret_val, msg )                                                                         \
    do {                                                                                                               \
        if (!( expr )) {                                                                                               \
            nc::assert_fail( #expr, msg, __FILE__, __LINE__ );                                                         \
            return ret_val;                                                                                            \
        }                                                                                                              \
    } while (0)

#define NC_VERIFY( ptr )                                                                                               \
    do {                                                                                                               \
        if (ptr == nullptr) {                                                                                          \
            nc::assert_fail( #ptr, "pointer is null reference", __FILE__, __LINE__ );                                  \
            std::abort();                                                                                              \
        }                                                                                                              \
    } while (0)

#endif
