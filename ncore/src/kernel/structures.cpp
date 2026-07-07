#include <cmath>
#include <limits>

#include <ncore/kernel/structures.h>

namespace nc {

Vec2::Vec2() : X( 0.0f ), Y( 0.0f ) {}

Vec2::Vec2( float x, float y ) : X( x ), Y( y ) {}

Vec2 Vec2::add( const Vec2& vec ) const
{
    return Vec2( this->X + vec.X, this->Y + vec.Y );
}

Vec2 Vec2::subtract( const Vec2& vec ) const
{
    return Vec2( this->X - vec.X, this->Y - vec.Y );
}

Vec2 Vec2::multiply( const Vec2& vec ) const
{
    return Vec2( this->X * vec.X, this->Y * vec.Y );
}

Vec2 Vec2::divide( const Vec2& vec ) const
{
    return Vec2( this->X / vec.X, this->Y / vec.Y );
}

Vec2 Vec2::add( const float f ) const
{
    return Vec2( this->X + f, this->Y + f );
}

Vec2 Vec2::subtract( const float f ) const
{
    return Vec2( this->X - f, this->Y - f );
}

Vec2 Vec2::multiply( const float f ) const
{
    return Vec2( this->X * f, this->Y * f );
}

Vec2 Vec2::divide( const float f ) const
{
    return Vec2( this->X / f, this->Y / f );
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
    this->X += vec.X;
    this->Y += vec.Y;
    return *this;
}

Vec2& Vec2::operator-=( const Vec2& vec )
{
    this->X -= vec.X;
    this->Y -= vec.Y;
    return *this;
}

Vec2& Vec2::operator*=( const Vec2& vec )
{
    this->X *= vec.X;
    this->Y *= vec.Y;
    return *this;
}

Vec2& Vec2::operator/=( const Vec2& vec )
{
    this->X /= vec.X;
    this->Y /= vec.Y;
    return *this;
}

Vec2& Vec2::zero()
{
    this->X = 0;
    this->Y = 0;
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
    X += f;
    Y += f;
    return *this;
}
Vec2& Vec2::operator-=( const float f )
{
    X -= f;
    Y -= f;
    return *this;
}
Vec2& Vec2::operator*=( const float f )
{
    X *= f;
    Y *= f;
    return *this;
}
Vec2& Vec2::operator/=( const float f )
{
    X /= f;
    Y /= f;
    return *this;
}

Vec2 Vec2::operator*( const int i ) const
{
    return Vec2( this->X * static_cast<float>( i ), this->Y * static_cast<float>( i ) );
}

Vec2 Vec2::operator-() const
{
    return Vec2( -X, -Y );
}

float Vec2::length() const
{
    return std::sqrt( X * X + Y * Y );
}

float Vec2::length_sqr() const
{
    return X * X + Y * Y;
}

Vec2 Vec2::lerp( const Vec2& vec1, const Vec2& vec2, float amount )
{
    amount = std::clamp( amount, 0.0f, 1.0f );
    return Vec2( vec1.X + ( vec2.X - vec1.X ) * amount, vec1.Y + ( vec2.Y - vec1.Y ) * amount );
}

bool Vec2::is_zero() const
{
    return length_sqr() < ( std::numeric_limits<float>::epsilon() * std::numeric_limits<float>::epsilon() );
}

} // namespace nc
