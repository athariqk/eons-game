#include <ncore/utils/four_cc.h>

namespace nc {

uint32_t FourCC::from_string( const std::string& str )
{
    if (str.length() != 4) {
        return 0;
    }

    return ( static_cast<uint32_t>( str[0] ) << 0 ) | ( static_cast<uint32_t>( str[1] ) << 8 ) |
           ( static_cast<uint32_t>( str[2] ) << 16 ) | ( static_cast<uint32_t>( str[3] ) << 24 );
}

std::string FourCC::to_string( uint32_t value )
{
    std::string result( 4, ' ' );

    result[0] = static_cast<char>( ( value >> 0 ) & 0xFF );
    result[1] = static_cast<char>( ( value >> 8 ) & 0xFF );
    result[2] = static_cast<char>( ( value >> 16 ) & 0xFF );
    result[3] = static_cast<char>( ( value >> 24 ) & 0xFF );

    return result;
}

bool FourCC::is_valid_fourcc( const std::string& str )
{
    if (str.length() != 4) {
        return false;
    }

    for (char c : str) {
        if (c < 32 || c > 126) {
            return false;
        }
    }
    return true;
}

bool FourCC::is_valid_fourcc( uint32_t value )
{
    for (int i = 0; i < 4; ++i) {
        char c = static_cast<char>( ( value >> ( i * 8 ) ) & 0xFF );
        if (c < 32 || c > 126) {
            return false;
        }
    }
    return true;
}

} // namespace nc
