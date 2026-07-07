#pragma once

namespace nc {

/**
 * @brief Static helpers for working with a four-code character encoding.
 *
 * See: https://en.wikipedia.org/wiki/FourCC
 */
struct FourCC {
    // Pack 4 characters into a 32-bit integer (little-endian)
    static uint32_t from_string( const std::string& str );
    static std::string to_string( uint32_t value );
    static bool is_valid_fourcc( const std::string& str );
    static bool is_valid_fourcc( uint32_t value );
};

} // namespace nc
