#pragma once

#include <concepts>
#include <type_traits>

namespace nc {

#define ENABLE_BITMASK( T )                                                                                            \
    constexpr T operator|( T lhs, T rhs ) noexcept                                                                     \
    {                                                                                                                  \
        return static_cast<T>(                                                                                         \
            static_cast<std::underlying_type_t<T>>( lhs ) | static_cast<std::underlying_type_t<T>>( rhs )              \
        );                                                                                                             \
    }                                                                                                                  \
    constexpr T operator&( T lhs, T rhs ) noexcept                                                                     \
    {                                                                                                                  \
        return static_cast<T>(                                                                                         \
            static_cast<std::underlying_type_t<T>>( lhs ) & static_cast<std::underlying_type_t<T>>( rhs )              \
        );                                                                                                             \
    }                                                                                                                  \
    constexpr T operator^( T lhs, T rhs ) noexcept                                                                     \
    {                                                                                                                  \
        return static_cast<T>(                                                                                         \
            static_cast<std::underlying_type_t<T>>( lhs ) ^ static_cast<std::underlying_type_t<T>>( rhs )              \
        );                                                                                                             \
    }                                                                                                                  \
    constexpr T operator~( T flag ) noexcept                                                                           \
    {                                                                                                                  \
        return static_cast<T>( ~static_cast<std::underlying_type_t<T>>( flag ) );                                      \
    }                                                                                                                  \
    constexpr T& operator|=( T& lhs, T rhs ) noexcept                                                                  \
    {                                                                                                                  \
        return lhs = lhs | rhs;                                                                                        \
    }                                                                                                                  \
    constexpr T& operator&=( T& lhs, T rhs ) noexcept                                                                  \
    {                                                                                                                  \
        return lhs = lhs & rhs;                                                                                        \
    }                                                                                                                  \
    constexpr T& operator^=( T& lhs, T rhs ) noexcept                                                                  \
    {                                                                                                                  \
        return lhs = lhs ^ rhs;                                                                                        \
    }                                                                                                                  \
    constexpr bool operator!( T flag ) noexcept                                                                        \
    {                                                                                                                  \
        return static_cast<std::underlying_type_t<T>>( flag ) == 0;                                                    \
    }                                                                                                                  \
    constexpr bool any( T flag ) noexcept                                                                              \
    {                                                                                                                  \
        return static_cast<std::underlying_type_t<T>>( flag ) != 0;                                                    \
    }                                                                                                                  \
    constexpr bool none( T flag ) noexcept                                                                             \
    {                                                                                                                  \
        return static_cast<std::underlying_type_t<T>>( flag ) == 0;                                                    \
    }

} // namespace nc
