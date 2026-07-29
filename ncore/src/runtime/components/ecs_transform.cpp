#include <ncore/core/matrix.h>
#include <ncore/runtime/components/ecs_transform.h>

namespace nc {

Mat4 nc::EcsTransform3D::get_matrix() const
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
    auto s   = 2 / ( rotation.w * rotation.w + rotation.v.length_sqr() );
    auto xs  = rotation.v.x * s;  // bs
    auto ys  = rotation.v.y * s;  // cs
    auto zs  = rotation.v.z * s;  // ds
    auto wx  = rotation.w * xs;   // ab
    auto xx  = rotation.v.x * xs; // bb
    auto yy  = rotation.v.y * ys; // cc
    auto wy  = rotation.w * ys;   // ac
    auto xy  = rotation.v.x * ys; // bc
    auto yz  = rotation.v.y * zs; // cd
    auto wz  = rotation.w * zs;   // ad
    auto xz  = rotation.v.x * zs; // bd
    auto zz  = rotation.v.z * zs; // dd
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
	// TransformMatrix = Translation * Rotation * Scale
	Mat4 matrix = Mat4(
		Vec4( scale.x * r00, scale.x * r10, scale.x * r20, 0 ),
		Vec4( scale.y * r01, scale.y * r11, scale.y * r21, 0 ),
		Vec4( scale.z * r02, scale.z * r12, scale.z * r22, 0 ),
		Vec4( translation.x, translation.y, translation.z, 1 )
	);
    // clang-format on

    return matrix;
}

} // namespace nc
