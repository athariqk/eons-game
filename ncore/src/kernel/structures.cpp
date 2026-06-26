#include <cmath>
#include <limits>

#include <ncore/kernel/structures.h>

namespace ncore {

Vec2::Vec2() : x( 0.0f ), y( 0.0f ) {}

Vec2::Vec2( float x, float y ) : x( x ), y( y ) {}

Vec2 Vec2::add( const Vec2& vec ) const
{
    return Vec2( this->x + vec.x, this->y + vec.y );
}

Vec2 Vec2::subtract( const Vec2& vec ) const
{
    return Vec2( this->x - vec.x, this->y - vec.y );
}

Vec2 Vec2::multiply( const Vec2& vec ) const
{
    return Vec2( this->x * vec.x, this->y * vec.y );
}

Vec2 Vec2::divide( const Vec2& vec ) const
{
    return Vec2( this->x / vec.x, this->y / vec.y );
}

Vec2 Vec2::add( const float f ) const
{
    return Vec2( this->x + f, this->y + f );
}

Vec2 Vec2::subtract( const float f ) const
{
    return Vec2( this->x - f, this->y - f );
}

Vec2 Vec2::multiply( const float f ) const
{
    return Vec2( this->x * f, this->y * f );
}

Vec2 Vec2::divide( const float f ) const
{
    return Vec2( this->x / f, this->y / f );
}

Vec2 operator+( const Vec2& v1, const Vec2& v2 )
{
    return v1.add( v2 );
}

Vec2 operator-( const Vec2& v1, const Vec2& v2 )
{
    return v1.subtract( v2 );
}

Vec2 operator*( const Vec2& v1, const Vec2& v2 )
{
    return v1.multiply( v2 );
}

Vec2 operator/( const Vec2& v1, const Vec2& v2 )
{
    return v1.divide( v2 );
}

Vec2& Vec2::operator+=( const Vec2& vec )
{
    this->x += vec.x;
    this->y += vec.y;
    return *this;
}

Vec2& Vec2::operator-=( const Vec2& vec )
{
    this->x -= vec.x;
    this->y -= vec.y;
    return *this;
}

Vec2& Vec2::operator*=( const Vec2& vec )
{
    this->x *= vec.x;
    this->y *= vec.y;
    return *this;
}

Vec2& Vec2::operator/=( const Vec2& vec )
{
    this->x /= vec.x;
    this->y /= vec.y;
    return *this;
}

Vec2& Vec2::zero()
{
    this->x = 0;
    this->y = 0;
    return *this;
}

Vec2 Vec2::operator+( const float f ) const
{
    return this->add( f );
}

Vec2 Vec2::operator-( const float f ) const
{
    return this->subtract( f );
}

Vec2 Vec2::operator*( const float f ) const
{
    return this->multiply( f );
}

Vec2 Vec2::operator/( const float f ) const
{
    return this->divide( f );
}

Vec2& Vec2::operator+=( const float f )
{
    x += f;
    y += f;
    return *this;
}
Vec2& Vec2::operator-=( const float f )
{
    x -= f;
    y -= f;
    return *this;
}
Vec2& Vec2::operator*=( const float f )
{
    x *= f;
    y *= f;
    return *this;
}
Vec2& Vec2::operator/=( const float f )
{
    x /= f;
    y /= f;
    return *this;
}

Vec2 Vec2::operator*( const int i ) const
{
    return Vec2( this->x * static_cast<float>( i ), this->y * static_cast<float>( i ) );
}

Vec2 Vec2::operator-() const
{
    return Vec2( -x, -y );
}

float Vec2::length() const
{
    return std::sqrt( x * x + y * y );
}

float Vec2::length_sqr() const
{
    return x * x + y * y;
}

Vec2 Vec2::lerp( const Vec2& vec1, const Vec2& vec2, float amount )
{
    amount = std::clamp( amount, 0.0f, 1.0f );
    return Vec2( vec1.x + ( vec2.x - vec1.x ) * amount, vec1.y + ( vec2.y - vec1.y ) * amount );
}

bool Vec2::is_zero() const
{
    return length_sqr() < ( std::numeric_limits<float>::epsilon() * std::numeric_limits<float>::epsilon() );
}

} // namespace ncore
