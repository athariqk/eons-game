// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: Color structure definition

#pragma once

namespace nc {

/**
 * @brief RGBA color.
 */
struct NCAPI Color {
    Color() {}
    Color( uint8_t R, uint8_t G, uint8_t B, uint8_t A ) : r( R ), g( G ), b( B ), a( A ) {}
    Color( uint32_t hex, uint8_t alpha = 200 )
    {
        unpack( hex, alpha );
    }

    static Color unpack( uint32_t hex, uint8_t alpha = 200 )
    {
        return {
            static_cast<uint8_t>( ( hex >> 16 ) & 0xFF ),
            static_cast<uint8_t>( ( hex >> 8 ) & 0xFF ),
            static_cast<uint8_t>( hex & 0xFF ),
            alpha,
        };
    }

    uint32_t pack() const
    {
        return ( static_cast<uint32_t>( a * 255.0f ) << 24 ) | ( static_cast<uint32_t>( b * 255.0f ) << 16 ) |
               ( static_cast<uint32_t>( g * 255.0f ) << 8 ) | ( static_cast<uint32_t>( r * 255.0f ) );
    }

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;

    bool operator==( const Color& other ) const
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    bool operator!=( const Color& other ) const
    {
        return !( *this == other );
    }

    NSTRUCT( Color, NC_F( Color, r ) NC_F( Color, g ) NC_F( Color, b ) NC_F( Color, a ) )
};

} // namespace nc
