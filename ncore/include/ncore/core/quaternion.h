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

    NSTRUCTV( Quaternion, NC_F( Quaternion, w ) NC_F( Quaternion, v ) )

    /**
     * @brief Initialize a new quaternion from angle axis.
     */
    Quaternion( float angle, Vec3 axis )
    {
        float rad = math::deg_to_rad( angle );

        w = std::cos( rad * 0.5f );
        v = axis * std::sin( rad * 0.5f );
    }

    /**
     * @brief Initialize a new quaternion from euler angles in ZYX sequence.
     *
     * Source:
     * https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles#Angles_(in_ZYX_sequence)_to_quaternion_conversion
     */
    Quaternion( Vec3 euler_angles )
    {
        w   = ( std::cos( euler_angles.x * 0.5f ) * std::cos( euler_angles.y * 0.5f ) *
                std::cos( euler_angles.z * 0.5f ) ) +
              ( std::sin( euler_angles.x * 0.5f ) * std::sin( euler_angles.y * 0.5f ) *
                std::sin( euler_angles.z * 0.5f ) );
        v.x = ( std::sin( euler_angles.x * 0.5f ) * std::cos( euler_angles.y * 0.5f ) *
                std::cos( euler_angles.z * 0.5f ) ) -
              ( std::cos( euler_angles.x * 0.5f ) * std::sin( euler_angles.y * 0.5f ) *
                std::sin( euler_angles.z * 0.5f ) );
        v.y = ( std::cos( euler_angles.x * 0.5f ) * std::sin( euler_angles.y * 0.5f ) *
                std::cos( euler_angles.z * 0.5f ) ) +
              ( std::sin( euler_angles.x * 0.5f ) * std::cos( euler_angles.y * 0.5f ) *
                std::sin( euler_angles.z * 0.5f ) );
        v.z = ( std::cos( euler_angles.x * 0.5f ) * std::cos( euler_angles.y * 0.5f ) *
                std::sin( euler_angles.z * 0.5f ) ) -
              ( std::sin( euler_angles.x * 0.5f ) * std::sin( euler_angles.y * 0.5f ) *
                std::cos( euler_angles.z * 0.5f ) );
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
        Vec3 unit_v = v_len > 0 ? Vec3::normalize( v ) : v; // unit vector
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
        Vec3 unit_v = v_len > 0 ? Vec3::normalize( v ) : v; // unit vector
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

    /**
     * @brief Returns a new Quaternion where all the values are between [0, 1].
     */
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

    /**
     * @brief Compose a 3x3 rotation matrix from quaternion.
     */
    static Mat3 to_rotation_matrix( const Quaternion& q )
    {
        // apparently people do this to avoid floating-point errors?
        // auto uq = Quaternion::normalize( rotation );

        // quaternion-to-matrix conversion
        // https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#From_a_quaternion_to_an_orthogonal_matrix
        // the slow version
        // auto r00 = ( uq.w * uq.w ) + ( uq.v.x * uq.v.x ) - ( uq.v.y * uq.v.y ) - ( uq.v.z * uq.v.z );
        // auto r01 = ( 2.0f * ( uq.v.x * uq.v.y ) ) - ( 2.0f * ( uq.w * uq.v.z ) );
        // auto r02 = ( 2.0f * ( uq.v.x * uq.v.z ) ) + ( 2.0f * ( uq.w * uq.v.y ) );
        // auto r10 = ( 2.0f * ( uq.v.x * uq.v.y ) ) + ( 2.0f * ( uq.w * uq.v.z ) );
        // auto r11 = ( uq.w * uq.w ) - ( uq.v.x * uq.v.x ) + ( uq.v.y * uq.v.y ) - ( uq.v.z * uq.v.z );
        // auto r12 = ( 2.0f * ( uq.v.y * uq.v.z ) ) - ( 2.0f * ( uq.w * uq.v.x ) );
        // auto r20 = ( 2.0f * ( uq.v.x * uq.v.z ) ) - ( 2.0f * ( uq.w * uq.v.y ) );
        // auto r21 = ( 2.0f * ( uq.v.y * uq.v.z ) ) + ( 2.0f * ( uq.w * uq.v.x ) );
        // auto r22 = ( uq.w * uq.w ) - ( uq.v.x * uq.v.x ) - ( uq.v.y * uq.v.y ) + ( uq.v.z * uq.v.z );
        // the fast version (no unit normalization)
        auto s   = 2 / ( q.w * q.w + q.v.length_sqr() );
        auto xs  = q.v.x * s;  // bs
        auto ys  = q.v.y * s;  // cs
        auto zs  = q.v.z * s;  // ds
        auto wx  = q.w * xs;   // ab
        auto xx  = q.v.x * xs; // bb
        auto yy  = q.v.y * ys; // cc
        auto wy  = q.w * ys;   // ac
        auto xy  = q.v.x * ys; // bc
        auto yz  = q.v.y * zs; // cd
        auto wz  = q.w * zs;   // ad
        auto xz  = q.v.x * zs; // bd
        auto zz  = q.v.z * zs; // dd
        auto r00 = 1 - yy - zz;
        auto r01 = xy - wz;
        auto r02 = xz + wy;
        auto r10 = xy + wz;
        auto r11 = 1 - xx - zz;
        auto r12 = yz - wx;
        auto r20 = xz - wy;
        auto r21 = yz + wx;
        auto r22 = 1 - xx - yy;

        // clang-format off
        return Mat3(
			Vec3( r00, r01, r02 ),
			Vec3( r10, r11, r12 ),
			Vec3( r20, r21, r22 )
		);
        // clang-format on
    }

    /**
     * @brief Convert quaternion to angles in ZYX sequence.
     *
     * Source:
     * https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles#Quaternion_to_angles_(in_ZYX_sequence)_conversion
     *
     * @param q Quaternion to be converted. Will be normalized before conversion.
     * @return The euler angles representation in ZYX sequence.
     */
    static Vec3 to_euler_angles( const Quaternion& q )
    {
        auto u = Quaternion::normalize( q );
        auto x = std::atan2f( 2 * ( u.w * u.v.x + u.v.y * u.v.z ), 1 - ( 2 * ( u.v.x * u.v.x + u.v.y * u.v.y ) ) );
        auto y = -math::HALF_PI + 2 * std::atan2f(
                                          std::sqrtf( 1 + ( 2 * ( u.w * u.v.y - u.v.x * u.v.z ) ) ),
                                          std::sqrtf( 1 - ( 2 * ( u.w * u.v.y - u.v.x * u.v.z ) ) )
                                      );
        auto z = std::atan2f( 2 * ( u.w * u.v.z + u.v.x * u.v.y ), 1 - ( 2 * ( u.v.y * u.v.y + u.v.z * u.v.z ) ) );
        return Vec3( x, y, z );
    }

    Quaternion normalize() const
    {
        return normalize( *this );
    }

    Mat3 to_rotation_matrix() const
    {
        return to_rotation_matrix( *this );
    }

    Vec3 to_euler_angles() const
    {
        return to_euler_angles( *this );
    }
};

} // namespace nc
