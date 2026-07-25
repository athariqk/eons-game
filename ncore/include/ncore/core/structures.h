// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: non-templated data structure types

#pragma once

#include <format>
#include <vector>

#include <ncore/core/types.h>

namespace nc {

/**
 * @brief A two-dimensional floating-point vector
 */
struct NCAPI Vec2 {
    float X, Y;

    Vec2();
    Vec2( float X, float Y );

    Vec2 add( const Vec2& vec ) const;
    Vec2 subtract( const Vec2& vec ) const;
    Vec2 multiply( const Vec2& vec ) const;
    Vec2 divide( const Vec2& vec ) const;

    Vec2 add( const float f ) const;
    Vec2 subtract( const float f ) const;
    Vec2 multiply( const float f ) const;
    Vec2 divide( const float f ) const;

    friend Vec2 operator+( const Vec2& v1, const Vec2& v2 );
    friend Vec2 operator-( const Vec2& v1, const Vec2& v2 );
    friend Vec2 operator*( const Vec2& v1, const Vec2& v2 );
    friend Vec2 operator/( const Vec2& v1, const Vec2& v2 );

    Vec2& operator+=( const Vec2& vec );
    Vec2& operator-=( const Vec2& vec );
    Vec2& operator*=( const Vec2& vec );
    Vec2& operator/=( const Vec2& vec );
    Vec2& zero();

    Vec2 operator+( const float f ) const;
    Vec2 operator-( const float f ) const;
    Vec2 operator*( const float f ) const;
    Vec2 operator/( const float f ) const;

    Vec2& operator+=( const float f );
    Vec2& operator-=( const float f );
    Vec2& operator*=( const float f );
    Vec2& operator/=( const float f );

    Vec2 operator*( const int i ) const;

    Vec2 operator-() const;

    float length() const;
    float length_sqr() const;

    static Vec2 lerp( const Vec2& vec1, const Vec2& vec2, float amount );

    bool is_zero() const;

    bool operator==( const Vec2& other ) const
    {
        return X == other.X && Y == other.Y;
    }
    bool operator!=( const Vec2& other ) const
    {
        return !( *this == other );
    }

    std::string to_string() const
    {
        return std::format( "Vec2D<X={},Y={}>", X, Y );
    }

    NSTRUCT( Vec2, NC_F( Vec2, X ) NC_F( Vec2, Y ) );
};

struct NCAPI Vec4 : Vec2 {
    float W, H;

    Vec4() : Vec2(), W( 0 ), H( 0 ) {}
    Vec4( float X, float Y, float p_W, float p_H ) : Vec2( X, Y ), W( p_W ), H( p_H ) {}

    bool is_zero() const
    {
        return Vec2::is_zero() && W == 0 && H == 0;
    }

    bool operator==( const Vec4& other ) const
    {
        return Vec2::operator==( other ) && W == other.W && H == other.H;
    }
    bool operator!=( const Vec4& other ) const
    {
        return !( *this == other );
    }

    NSTRUCT( Vec4, NC_F( Vec4, W ) NC_F( Vec4, H ) );
};

struct NCAPI Color {
    Color() {}
    Color( uint8_t r, uint8_t g, uint8_t b, uint8_t a ) : r( r ), g( g ), b( b ), a( a ) {}
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

    NSTRUCT( Color, NC_F( Color, r ) NC_F( Color, g ) NC_F( Color, b ) NC_F( Color, a ) );
};

} // namespace nc
