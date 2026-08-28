#pragma once

#include <concepts>
#include <type_traits>

namespace nc {

#define ENABLE_BITMASK( T )                                                                                            \
    constexpr T operator|( T lhs, T rhs )                                                                              \
    {                                                                                                                  \
        return static_cast<T>(                                                                                         \
            static_cast<std::underlying_type_t<T>>( lhs ) | static_cast<std::underlying_type_t<T>>( rhs )              \
        );                                                                                                             \
    }                                                                                                                  \
    constexpr T operator&( T lhs, T rhs )                                                                              \
    {                                                                                                                  \
        return static_cast<T>(                                                                                         \
            static_cast<std::underlying_type_t<T>>( lhs ) & static_cast<std::underlying_type_t<T>>( rhs )              \
        );                                                                                                             \
    }                                                                                                                  \
    constexpr T operator~( T flag )                                                                                    \
    {                                                                                                                  \
        return static_cast<T>( ~static_cast<std::underlying_type_t<T>>( flag ) );                                      \
    }

} // namespace nc
