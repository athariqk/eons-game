#pragma once

#include "vector.h"

namespace nc {

/**
 * @brief Mat4 is a 4x4 matrix with column-major order.
 */
struct NCAPI Mat4 {
    Vec4 col0, col1, col2, col3;

    Mat4() = default;
    Mat4( Vec4 pcol0, Vec4 pcol1, Vec4 pcol2, Vec4 pcol3 ) : col0( pcol0 ), col1( pcol1 ), col2( pcol2 ), col3( pcol3 )
    {}

    float* data()
    {
        return &col0.x;
    }
    const float* data() const
    {
        return &col0.x;
    }

    static Mat4 identity()
    {
        return Mat4(
            Vec4( 1.0f, 0.0f, 0.0f, 0.0f ), Vec4( 0.0f, 1.0f, 0.0f, 0.0f ), Vec4( 0.0f, 0.0f, 1.0f, 0.0f ),
            Vec4( 0.0f, 0.0f, 0.0f, 1.0f )
        );
    }

    static Mat4 orthographic( float left, float right, float bottom, float top, float near_, float far_ )
    {
        float rml = right - left;
        float tmb = top - bottom;
        float fmn = far_ - near_;

        return Mat4(
            Vec4( 2.0f / rml, 0.0f, 0.0f, 0.0f ), Vec4( 0.0f, 2.0f / tmb, 0.0f, 0.0f ),
            Vec4( 0.0f, 0.0f, -2.0f / fmn, 0.0f ),
            Vec4( -( right + left ) / rml, -( top + bottom ) / tmb, -( far_ + near_ ) / fmn, 1.0f )
        );
    }

    /**
     * @brief Perform M * M operation.
     */
    Mat4 multiply( const Mat4& rhs ) const
    {
        return Mat4( multiply( rhs.col0 ), multiply( rhs.col1 ), multiply( rhs.col2 ), multiply( rhs.col3 ) );
    }

    /**
     * @brief Perform M * v operation.
     */
    Vec4 multiply( const Vec4& v ) const
    {
        return Vec4(
            col0.x * v.x + col1.x * v.y + col2.x * v.z + col3.x * v.w,
            col0.y * v.x + col1.y * v.y + col2.y * v.z + col3.y * v.w,
            col0.z * v.x + col1.z * v.y + col2.z * v.z + col3.z * v.w,
            col0.w * v.x + col1.w * v.y + col2.w * v.z + col3.w * v.w
        );
    }

    Vec3 multiply( const Vec3& v ) const
    {
        Vec4 r = multiply( Vec4( v.x, v.y, v.z, 1.0f ) );
        return Vec3( r.x, r.y, r.z );
    }

    Mat4 multiply( float scalar ) const
    {
        return Mat4( col0 * scalar, col1 * scalar, col2 * scalar, col3 * scalar );
    }

    Mat4 operator*( float scalar ) const
    {
        return multiply( scalar );
    }

    Mat4& operator*=( float scalar )
    {
        *this = multiply( scalar );
        return *this;
    }

    friend Mat4 operator*( float scalar, const Mat4& m )
    {
        return m.multiply( scalar );
    }

    Mat4 operator*( const Mat4& rhs ) const
    {
        return multiply( rhs );
    }

    Mat4& operator*=( const Mat4& rhs )
    {
        *this = multiply( rhs );
        return *this;
    }

    Vec3 operator*( const Vec3& rhs ) const
    {
        return multiply( rhs );
    }

    bool operator==( const Mat4& rhs ) const
    {
        return col0 == rhs.col0 && col1 == rhs.col1 && col2 == rhs.col2 && col3 == rhs.col3;
    }

    bool operator!=( const Mat4& rhs ) const
    {
        return !( *this == rhs );
    }

    NSTRUCT( Mat4, NC_F( Mat4, col0 ) NC_F( Mat4, col1 ) NC_F( Mat4, col2 ) NC_F( Mat4, col3 ) )
};

} // namespace nc
