#pragma once

#include <ncore/utils/math.h>

#include "vector.h"

namespace nc {

struct NCAPI Rect {
    float x = 0, y = 0, w = 0, h = 0;

    Rect() = default;
    Rect( float px, float py, float pw, float ph ) : x( px ), y( py ), w( pw ), h( ph ) {}

    float left() const
    {
        return x;
    }
    float right() const
    {
        return x + w;
    }
    float top() const
    {
        return y;
    }
    float bottom() const
    {
        return y + h;
    }

    Vec2 position() const
    {
        return Vec2( x, y );
    }
    Vec2 size() const
    {
        return Vec2( w, h );
    }
    Vec2 center() const
    {
        return Vec2( x + w * 0.5f, y + h * 0.5f );
    }

    bool contains( const Vec2& point ) const
    {
        return point.x >= x && point.x <= x + w && point.y >= y && point.y <= y + h;
    }

    bool intersects( const Rect& other ) const
    {
        return x < other.x + other.w && x + w > other.x && y < other.y + other.h && y + h > other.y;
    }

    Rect& expand( const Vec2& amount )
    {
        x -= amount.x;
        y -= amount.y;
        w += amount.x * 2.0f;
        h += amount.y * 2.0f;
        return *this;
    }

    Rect& merge( const Rect& other )
    {
        float nx = x < other.x ? x : other.x;
        float ny = y < other.y ? y : other.y;
        w        = ( x + w > other.x + other.w ? x + w : other.x + other.w ) - nx;
        h        = ( y + h > other.y + other.h ? y + h : other.y + other.h ) - ny;
        x        = nx;
        y        = ny;
        return *this;
    }

    static Rect from_position_size( const Vec2& pos, const Vec2& size )
    {
        return Rect( pos.x, pos.y, size.x, size.y );
    }

    static Rect from_min_max( const Vec2& min, const Vec2& max )
    {
        return Rect( min.x, min.y, max.x - min.x, max.y - min.y );
    }

    bool operator==( const Rect& other ) const
    {
        return math::is_equal_approx( x, other.x ) && math::is_equal_approx( y, other.y ) &&
               math::is_equal_approx( w, other.w ) && math::is_equal_approx( h, other.h );
    }

    bool operator!=( const Rect& other ) const
    {
        return !( *this == other );
    }

    NSTRUCT( Rect, NC_F( Rect, x ) NC_F( Rect, y ) NC_F( Rect, w ) NC_F( Rect, h ) )
};

} // namespace nc
