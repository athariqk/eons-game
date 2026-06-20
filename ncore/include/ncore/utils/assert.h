#pragma once

#include <cstdlib>

#if defined(_MSC_VER)
#define NC_DEBUGBREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define NC_DEBUGBREAK() __builtin_trap()
#else
#define NC_DEBUGBREAK() (void) 0
#endif

namespace ncore::log {
void handle_assert(const char *expr, const char *msg, const char *file, int line);
}

#ifdef DEBUG
#define NC_ASSERT(expr, msg)                                                                                           \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            ncore::log::handle_assert(#expr, msg, __FILE__, __LINE__);                                                 \
            NC_DEBUGBREAK();                                                                                           \
        }                                                                                                              \
    } while (0)

#define NC_ASSERT_RET(expr, msg)                                                                                       \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            ncore::log::handle_assert(#expr, msg, __FILE__, __LINE__);                                                 \
            NC_DEBUGBREAK();                                                                                           \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define NC_ASSERT_RETVAL(expr, ret_val, msg)                                                                           \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            ncore::log::handle_assert(#expr, msg, __FILE__, __LINE__);                                                 \
            NC_DEBUGBREAK();                                                                                           \
            return ret_val;                                                                                            \
        }                                                                                                              \
    } while (0)

#else
#define NC_ASSERT(expr, msg)                                                                                           \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            ncore::log::handle_assert(#expr, msg, __FILE__, __LINE__);                                                 \
            std::abort();                                                                                              \
        }                                                                                                              \
    } while (0)

#define NC_ASSERT_RET(expr, msg)                                                                                       \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            ncore::log::handle_assert(#expr, msg, __FILE__, __LINE__);                                                 \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define NC_ASSERT_RETVAL(expr, ret_val, msg)                                                                           \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            ncore::log::handle_assert(#expr, msg, __FILE__, __LINE__);                                                 \
            return ret_val;                                                                                            \
        }                                                                                                              \
    } while (0)
#endif
