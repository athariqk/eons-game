// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: Vector data structure types

#pragma once

#include <ncore.h>
#include <ncore/utils/math.h>

#include "types.h"

namespace nc {

/**
* @brief CommonVectorOps define shared math operations for
* N-dimension vectors of type T.
*/
template<typename Derived, typename T, std::size_t N>
struct CommonVectorOps {
private:
    constexpr Derived& self()
    {
        return static_cast<Derived&>( *this );
    }

    constexpr const Derived& self() const
    {
        return static_cast<const Derived&>( *this );
    }

public:
    Derived add( const Derived& rhs ) const
    {
        Derived out{};
        for (std::size_t i = 0; i < N; ++i)
            out.data()[i] = self().data()[i] + rhs.data()[i];
        return out;
    }

    Derived subtract( const Derived& rhs ) const
    {
        Derived out{};
        for (std::size_t i = 0; i < N; ++i)
            out.data()[i] = self().data()[i] - rhs.data()[i];
        return out;
    }

    Derived multiply( const Derived& rhs ) const
    {
        Derived out{};
        for (std::size_t i = 0; i < N; ++i)
            out.data()[i] = self().data()[i] * rhs.data()[i];
        return out;
    }

    Derived divide( const Derived& rhs ) const
    {
        Derived out{};
        for (std::size_t i = 0; i < N; ++i)
            out.data()[i] = self().data()[i] / rhs.data()[i];
        return out;
    }

    T dot( const Derived& rhs ) const
    {
        T out{};
        for (std::size_t i = 0; i < N; ++i)
            out += self().data()[i] * rhs.data()[i];
        return out;
    }

    Derived add( T value ) const
    {
        Derived out{};
        for (std::size_t i = 0; i < N; ++i)
            out.data()[i] = self().data()[i] + value;
        return out;
    }

    Derived subtract( T value ) const
    {
        Derived out{};
        for (std::size_t i = 0; i < N; ++i)
            out.data()[i] = self().data()[i] - value;
        return out;
    }

    Derived multiply( T value ) const
    {
        Derived out{};
        for (std::size_t i = 0; i < N; ++i)
            out.data()[i] = self().data()[i] * value;
        return out;
    }

    Derived divide( T value ) const
    {
        Derived out{};
        for (std::size_t i = 0; i < N; ++i)
            out.data()[i] = self().data()[i] / value;
        return out;
    }

    friend Derived operator+( const Derived& lhs, const Derived& rhs )
    {
        return lhs.add( rhs );
    }

    friend Derived operator-( const Derived& lhs, const Derived& rhs )
    {
        return lhs.subtract( rhs );
    }

    friend Derived operator*( const Derived& lhs, const Derived& rhs )
    {
        return lhs.multiply( rhs );
    }

    friend Derived operator/( const Derived& lhs, const Derived& rhs )
    {
        return lhs.divide( rhs );
    }

    Derived& operator+=( const Derived& rhs )
    {
        for (std::size_t i = 0; i < N; ++i)
            self().data()[i] += rhs.data()[i];
        return self();
    }

    Derived& operator-=( const Derived& rhs )
    {
        for (std::size_t i = 0; i < N; ++i)
            self().data()[i] -= rhs.data()[i];
        return self();
    }

    Derived& operator*=( const Derived& rhs )
    {
        for (std::size_t i = 0; i < N; ++i)
            self().data()[i] *= rhs.data()[i];
        return self();
    }

    Derived& operator/=( const Derived& rhs )
    {
        for (std::size_t i = 0; i < N; ++i)
            self().data()[i] /= rhs.data()[i];
        return self();
    }

    Derived operator+( T value ) const
    {
        return add( value );
    }

    Derived operator-( T value ) const
    {
        return subtract( value );
    }

    Derived operator*( T value ) const
    {
        return multiply( value );
    }

    Derived operator/( T value ) const
    {
        return divide( value );
    }

    Derived operator*( int value ) const
    {
        return multiply( static_cast<T>( value ) );
    }

    Derived& operator+=( T value )
    {
        for (std::size_t i = 0; i < N; ++i)
            self().data()[i] += value;
        return self();
    }

    Derived& operator-=( T value )
    {
        for (std::size_t i = 0; i < N; ++i)
            self().data()[i] -= value;
        return self();
    }

    Derived& operator*=( T value )
    {
        for (std::size_t i = 0; i < N; ++i)
            self().data()[i] *= value;
        return self();
    }

    Derived& operator/=( T value )
    {
        for (std::size_t i = 0; i < N; ++i)
            self().data()[i] /= value;
        return self();
    }

    Derived operator-() const
    {
        Derived out{};
        for (std::size_t i = 0; i < N; ++i)
            out.data()[i] = -self().data()[i];
        return out;
    }

    /**
	* @brief Set value of all components to zero.
	*/
    Derived& zero()
    {
        for (std::size_t i = 0; i < N; ++i)
            self().data()[i] = T{};
        return self();
    }

    T length_sqr() const
    {
        T result{};
        for (std::size_t i = 0; i < N; ++i) {
            auto& c = self().data()[i];
            result += c * c;
        }
        return result;
    }

    T length() const
    {
        return std::sqrt( length_sqr() );
    }

    bool is_zero() const
    {
        return length_sqr() < std::numeric_limits<T>::epsilon();
    }

    bool is_equal_approx( const Derived& rhs )
    {
        for (std::size_t i = 0; i < N; ++i) {
            if (!math::is_equal_approx( self().data()[i], rhs.data()[i] ))
                return false;
        }
        return true;
    }

    static Derived lerp( const Derived& a, const Derived& b, T amount )
    {
        amount = std::clamp( amount, T( 0 ), T( 1 ) );
        Derived out{};
        for (std::size_t i = 0; i < N; ++i) {
            out.data()[i] = a.data()[i] + ( b.data()[i] - a.data()[i] ) * amount;
        }
        return out;
    }

    static Derived normalize( const Derived& v )
    {
        auto m = v.length();
        Derived out{};
        if (m <= 0)
            return out;
        for (std::size_t i = 0; i < N; ++i) {
            out.data()[i] = v.data()[i] / m;
        }
        return out;
    }

    Derived normalize()
    {
        return normalize( self() );
    }

    bool operator==( const Derived& rhs ) const
    {
        for (std::size_t i = 0; i < N; ++i) {
            if (self().data()[i] != rhs.data()[i])
                return false;
        }
        return true;
    }

    bool operator!=( const Derived& rhs ) const
    {
        return !( *this == rhs );
    }
};

/**
 * @brief 2 components vector of single-precision floating-point numbers.
 */
struct NCAPI Vec2 : CommonVectorOps<Vec2, float, 2> {
    float x = 0, y = 0;

    Vec2() = default;
    Vec2( float px, float py ) : x( px ), y( py ) {}

    float* data()
    {
        return &x;
    }
    const float* data() const
    {
        return &x;
    }

    NSTRUCT( Vec2, NC_F( Vec2, x ) NC_F( Vec2, y ) )
};

/**
 * @brief 3 components vector of single-precision floating-point numbers.
 */
struct NCAPI Vec3 : CommonVectorOps<Vec3, float, 3> {
    float x = 0, y = 0, z = 0;

    Vec3() = default;
    Vec3( float px, float py, float pz ) : x( px ), y( py ), z( pz ) {}

    float* data()
    {
        return &x;
    }
    const float* data() const
    {
        return &x;
    }

    Vec3 cross( const Vec3& rhs ) const
    {
        return { y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x };
    }

    static Vec3 up()
    {
        return Vec3( 0, 1, 0 );
    }

    static Vec3 down()
    {
        return Vec3( 0, -1, 0 );
    }

    static Vec3 forward()
    {
        return Vec3( 0, 0, -1 );
    }

    static Vec3 backward()
    {
        return Vec3( 0, 0, 1 );
    }

    static Vec3 right()
    {
        return Vec3( 1, 0, 0 );
    }

    static Vec3 left()
    {
        return Vec3( -1, 0, 0 );
    }

    NSTRUCT( Vec3, NC_F( Vec3, x ) NC_F( Vec3, y ) NC_F( Vec3, z ) )
};

/**
 * @brief 4 components vector of single-precision floating-point numbers.
 */
struct Vec4 : CommonVectorOps<Vec4, float, 4> {
    float x = 0, y = 0, z = 0, w = 0;

    Vec4() = default;
    Vec4( float px, float py, float pz, float pw ) : x( px ), y( py ), z( pz ), w( pw ) {}

    float* data()
    {
        return &x;
    }
    const float* data() const
    {
        return &x;
    }

    NSTRUCT( Vec4, NC_F( Vec4, x ) NC_F( Vec4, y ) NC_F( Vec4, z ) NC_F( Vec4, w ) )
};

} // namespace nc
