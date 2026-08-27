#pragma once

#include <ncore/utils/math.h>

#include "vector.h"

namespace nc {

struct NCAPI Rect2f {
    float x = 0, y = 0, w = 0, h = 0;

    Rect2f() = default;
    Rect2f( float px, float py, float pw, float ph ) : x( px ), y( py ), w( pw ), h( ph ) {}

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

    Vec2f position() const
    {
        return Vec2f( x, y );
    }
    Vec2f size() const
    {
        return Vec2f( w, h );
    }
    Vec2f center() const
    {
        return Vec2f( x + w * 0.5f, y + h * 0.5f );
    }

    bool contains( const Vec2f& point ) const
    {
        return point.x >= x && point.x <= x + w && point.y >= y && point.y <= y + h;
    }

    bool intersects( const Rect2f& other ) const
    {
        return x < other.x + other.w && x + w > other.x && y < other.y + other.h && y + h > other.y;
    }

    bool has_area() const
    {
        return w > 0 && h > 0;
    }

    Rect2f& expand( const Vec2f& amount )
    {
        x -= amount.x;
        y -= amount.y;
        w += amount.x * 2.0f;
        h += amount.y * 2.0f;
        return *this;
    }

    Rect2f& merge( const Rect2f& other )
    {
        float nx = x < other.x ? x : other.x;
        float ny = y < other.y ? y : other.y;
        w        = ( x + w > other.x + other.w ? x + w : other.x + other.w ) - nx;
        h        = ( y + h > other.y + other.h ? y + h : other.y + other.h ) - ny;
        x        = nx;
        y        = ny;
        return *this;
    }

    static Rect2f from_position_size( const Vec2f& pos, const Vec2f& size )
    {
        return Rect2f( pos.x, pos.y, size.x, size.y );
    }

    static Rect2f from_min_max( const Vec2f& min, const Vec2f& max )
    {
        return Rect2f( min.x, min.y, max.x - min.x, max.y - min.y );
    }

    bool operator==( const Rect2f& other ) const
    {
        return math::is_equal_approx( x, other.x ) && math::is_equal_approx( y, other.y ) &&
               math::is_equal_approx( w, other.w ) && math::is_equal_approx( h, other.h );
    }

    bool operator!=( const Rect2f& other ) const
    {
        return !( *this == other );
    }

    NSTRUCTV( Rect2f, NC_F( Rect2f, x ), NC_F( Rect2f, y ), NC_F( Rect2f, w ), NC_F( Rect2f, h ) )
};

struct NCAPI Rect2i {
    int x = 0, y = 0, w = 0, h = 0;

    Rect2i() = default;
    Rect2i( int px, int py, int pw, int ph ) : x( px ), y( py ), w( pw ), h( ph ) {}

    int left() const
    {
        return x;
    }
    int right() const
    {
        return x + w;
    }
    int top() const
    {
        return y;
    }
    int bottom() const
    {
        return y + h;
    }

    Vec2i position() const
    {
        return Vec2i( x, y );
    }
    Vec2i size() const
    {
        return Vec2i( w, h );
    }
    Vec2i center() const
    {
        return Vec2i( x + w / 2, y + h / 2 );
    }

    bool contains( const Vec2i& point ) const
    {
        return point.x >= x && point.x <= x + w && point.y >= y && point.y <= y + h;
    }

    bool intersects( const Rect2i& other ) const
    {
        return x < other.x + other.w && x + w > other.x && y < other.y + other.h && y + h > other.y;
    }

    bool has_area() const
    {
        return w > 0 && h > 0;
    }

    Rect2i& expand( const Vec2i& amount )
    {
        x -= amount.x;
        y -= amount.y;
        w += amount.x * 2;
        h += amount.y * 2;
        return *this;
    }

    Rect2i& merge( const Rect2i& other )
    {
        int nx = x < other.x ? x : other.x;
        int ny = y < other.y ? y : other.y;
        w      = ( x + w > other.x + other.w ? x + w : other.x + other.w ) - nx;
        h      = ( y + h > other.y + other.h ? y + h : other.y + other.h ) - ny;
        x      = nx;
        y      = ny;
        return *this;
    }

    static Rect2i from_position_size( const Vec2i& pos, const Vec2i& size )
    {
        return Rect2i( pos.x, pos.y, size.x, size.y );
    }

    static Rect2i from_min_max( const Vec2i& min, const Vec2i& max )
    {
        return Rect2i( min.x, min.y, max.x - min.x, max.y - min.y );
    }

    bool operator==( const Rect2i& other ) const
    {
        return x == other.x && y == other.y && w == other.w && h == other.h;
    }

    bool operator!=( const Rect2i& other ) const
    {
        return !( *this == other );
    }

    NSTRUCTV( Rect2i, NC_F( Rect2i, x ), NC_F( Rect2i, y ), NC_F( Rect2i, w ), NC_F( Rect2i, h ) )
};

} // namespace nc
