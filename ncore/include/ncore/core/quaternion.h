#pragma once

#include <numbers>

#include "vector.h"

namespace nc {

/**
 * @brief From online sources: 3D quaternion is a 4-number
 * system with one real number and three complex numbers.
 * Used for gimbal-lock free and efficient rotations.
 *
 * Useful references:
 * https://imadrahmoune.com/rotations-with-quaternions/
 * http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/
 */
struct NCAPI Quaternion {
    float w = 0;
    Vec3 v  = Vec3();

    Quaternion() = default;

    NSTRUCT( Quaternion, NC_F( Quaternion, w ) NC_F( Quaternion, v ) )

    /**
     * @brief Initialize a new quaternion from angle axis.
     */
    Quaternion( float angle, Vec3 axis )
    {
        float rad = math::deg_to_rad( angle );

        w = std::cos( rad * 0.5f );
        v = axis * std::sin( rad * 0.5f );
    }

    Quaternion multiply( const Quaternion& q ) const
    {
        Quaternion r;
        r.w = w * q.w - v.dot( q.v );
        r.v = v * q.w + q.v * w + v.cross( q.v );
        return r;
    }

    Vec3 multiply( const Vec3& vec ) const
    {
        Quaternion p;
        p.w     = 0;
        p.v     = vec;
        auto& q = *this;
        auto r  = q * p * q.inverse();
        return r.v;
    }

    Quaternion multiply( float scalar ) const
    {
        Quaternion p;
        p.w = w;
        p.v = v;
        p.w *= scalar;
        p.v *= scalar;
        return p;
    }

    Quaternion divide( float scalar ) const
    {
        Quaternion p;
        p.w = w;
        p.v = v;
        p.w /= scalar;
        p.v /= scalar;
        return p;
    }

    Quaternion exp() const
    {
        float v_len = v.length();
        float v_sin = std::sin( v_len );
        float w_exp = std::expf( w );
        Vec3 unit_v = v_len > 0 ? Vec3::normalize( v ) : v;
        Quaternion r;
        r.w   = std::cos( v_len ) * w_exp;
        r.v.x = unit_v.x * v_sin * w_exp;
        r.v.y = unit_v.y * v_sin * w_exp;
        r.v.z = unit_v.z * v_sin * w_exp;
        return r;
    }

    Quaternion log() const
    {
        float m     = length();
        float a     = std::acosf( w / m );
        float v_len = v.length();
        Vec3 unit_v = v_len > 0 ? Vec3::normalize( v ) : v;
        Quaternion r;
        r.w   = std::log( m );
        r.v.x = unit_v.x * a;
        r.v.y = unit_v.y * a;
        r.v.z = unit_v.z * a;
        return r;
    }

    Quaternion pow( float n ) const
    {
        Quaternion r = log() * n;
        return r.exp();
    }

    Quaternion operator*( const Quaternion& rhs ) const
    {
        return multiply( rhs );
    }

    Vec3 operator*( const Vec3& rhs ) const
    {
        return multiply( rhs );
    }

    friend Vec3 operator*( const Vec3 vec, const Quaternion& quat )
    {
        return quat.multiply( vec );
    }

    Quaternion operator*( float rhs ) const
    {
        return multiply( rhs );
    }

    Quaternion operator/( float scalar ) const
    {
        return divide( scalar );
    }

    Quaternion operator^( float n ) const
    {
        return pow( n );
    }

    Quaternion operator-() const
    {
        return inverse();
    }

    Quaternion& operator*=( const Quaternion& rhs )
    {
        *this = multiply( rhs );
        return *this;
    }

    Quaternion& operator*=( float scalar )
    {
        w *= scalar;
        v *= scalar;
        return *this;
    }

    Quaternion& operator/=( float scalar )
    {
        w /= scalar;
        v /= scalar;
        return *this;
    }

    Quaternion inverse() const
    {
        Quaternion result;
        result.w   = w;
        result.v.x = -v.x;
        result.v.y = -v.y;
        result.v.z = -v.z;
        return result;
    }

    float dot( const Quaternion& q ) const
    {
        return w * q.w + v.dot( q.v );
    }

    float length_sqr() const
    {
        return w * w + v.length_sqr();
    }

    float length() const
    {
        return std::sqrt( length_sqr() );
    }

    static Quaternion identity()
    {
        Quaternion q;
        q.w = 1;
        return q;
    }

    static Quaternion normalize( const Quaternion& q )
    {
        // unit quaternion = q / sqrt(length_of_q_squared)
        // https://math.stackexchange.com/questions/1703466/normalizing-a-quaternion
        return q / q.length();
    }

    static Quaternion slerp( const Quaternion& q1, const Quaternion& q2, float t )
    {
        t = std::clamp( t, 0.0f, 1.0f );

        auto target = q2;
        if (q1.dot( target ) < 0)
            target *= -1;
        auto factor = ( q1.inverse() * target ) ^ t;

        return q1 * factor;
    }
};

} // namespace nc
