#pragma once

#include "vector.h"

namespace nc {

/**
 * @brief Mat3 is a 3x3 matrix with column-major order.
 */
struct NCAPI Mat3 {
    Vec3 col0, col1, col2;

    Mat3() = default;
    Mat3( Vec3 pcol0, Vec3 pcol1, Vec3 pcol2 ) : col0( pcol0 ), col1( pcol1 ), col2( pcol2 ) {}

    static Mat3 identity()
    {
        // clang-format off
        return Mat3(
            Vec3( 1.0f, 0.0f, 0.0f ),
			Vec3( 0.0f, 1.0f, 0.0f ),
			Vec3( 0.0f, 0.0f, 1.0f )
        );
        // clang-format on
    }

    NSTRUCTV( Mat3, NC_F( Mat3, col0 ) NC_F( Mat3, col1 ) NC_F( Mat3, col2 ) )
};

/**
 * @brief Mat4 is a 4x4 matrix with column-major order.
 */
struct NCAPI Mat4 {
    Vec4 col0, col1, col2, col3;

    Mat4() = default;
    Mat4( const Vec4& pcol0, const Vec4& pcol1, const Vec4& pcol2, const Vec4& pcol3 ) :
        col0( pcol0 ), col1( pcol1 ), col2( pcol2 ), col3( pcol3 )
    {}

    float* data()
    {
        return &col0.x;
    }
    const float* data() const
    {
        return &col0.x;
    }

    float read( int column, int row )
    {
        return data()[column * 4 + row];
    }

    float* write( int column, int row )
    {
        return data() + column * 4 + row;
    }

    static Mat4 identity()
    {
        // clang-format off
        return Mat4(
            Vec4( 1.0f, 0.0f, 0.0f, 0.0f ),
			Vec4( 0.0f, 1.0f, 0.0f, 0.0f ),
			Vec4( 0.0f, 0.0f, 1.0f, 0.0f ),
            Vec4( 0.0f, 0.0f, 0.0f, 1.0f )
        );
        // clang-format on
    }

    static Mat4 orthographic( float left, float right, float bottom, float top, float near_, float far_ )
    {
        float rml = right - left;
        float tmb = top - bottom;
        float fmn = far_ - near_;

        // clang-format off
        return Mat4(
            Vec4( 2.0f / rml, 0.0f, 0.0f, 0.0f ),
			Vec4( 0.0f, 2.0f / tmb, 0.0f, 0.0f ),
            Vec4( 0.0f, 0.0f, -2.0f / fmn, 0.0f ),
            Vec4( -( right + left ) / rml, -( top + bottom ) / tmb, -( far_ + near_ ) / fmn, 1.0f )
        );
        // clang-format on
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

    /**
     * @brief Perform M * v operation.
     */
    Vec3 multiply( const Vec3& v ) const
    {
        Vec4 r = multiply( Vec4( v.x, v.y, v.z, 1.0f ) );
        return Vec3( r.x, r.y, r.z );
    }

    Mat4 multiply( float scalar ) const
    {
        return Mat4( col0 * scalar, col1 * scalar, col2 * scalar, col3 * scalar );
    }

    /**
     * @brief Affine inverse exploits the structure of transform
     * matrices to perform faster inverse matrix computation.
     *
     * https://stackoverflow.com/a/2625420
     */
    Mat4 affine_inverse() const
    {
        // clang-format off
		// take the top-left block.
        Vec3 block[3] = {
            Vec3( col0.x, col0.y, col0.z ),
			Vec3( col1.x, col1.y, col1.z ),
			Vec3( col2.x, col2.y, col2.z )
        };
        // clang-format on

        // we calculate its determinant by using vector cross product.
        auto det = block[0].dot( block[1].cross( block[2] ) );
        NC_ASSERT( det > 0, "Matrix is singular (non-invertible)" );

        // find its adjoint matrix.
        auto adj0 = block[1].cross( block[2] );
        auto adj1 = block[2].cross( block[0] );
        auto adj2 = block[0].cross( block[1] );

        // inverse formula: 1 / det * (M)
        auto recp = 1 / det;
        auto inv0 = adj0 * recp;
        auto inv1 = adj1 * recp;
        auto inv2 = adj2 * recp;

        // final inverse translation vector calc.
        auto xlation = Vec3( col3.x, col3.y, col3.z );
        auto b0      = -inv0.dot( xlation );
        auto b1      = -inv1.dot( xlation );
        auto b2      = -inv2.dot( xlation );

        // clang-format off
		// reconstruct 4x4 matrix.
		return Mat4{
            Vec4( inv0.x, inv1.x, inv2.x, 0),
			Vec4( inv0.y, inv1.y, inv2.y, 0),
			Vec4( inv0.z, inv1.z, inv2.z, 0),
			Vec4( b0,     b1,     b2,     1)
		};
        // clang-format on
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

    static Mat4 ortho_normalize( const Mat4& m )
    {
        return Mat4( m.col0.normalize(), m.col1.normalize(), m.col2.normalize(), m.col3 );
    }

    Mat4 ortho_normalize() const
    {
        return Mat4::ortho_normalize( *this );
    }

    NSTRUCTV( Mat4, NC_F( Mat4, col0 ) NC_F( Mat4, col1 ) NC_F( Mat4, col2 ) NC_F( Mat4, col3 ) )
};

} // namespace nc
