#include <ncore/core/matrix.h>
#include <ncore/core/vector.h>
#include <ncore/runtime/components/transform.h>

namespace nc {

Mat4 nc::Transform3DComponent::to_matrix() const
{
    Mat3 r = rotation.to_rotation_matrix();

    // clang-format off
	// TransformMatrix = Translation * Rotation * scale
	Mat4 matrix = Mat4(
		Vec4f( scale.x * r.col0.x, scale.x * r.col1.x, scale.x * r.col2.x, 0 ),
		Vec4f( scale.y * r.col0.y, scale.y * r.col1.y, scale.y * r.col2.y, 0 ),
		Vec4f( scale.z * r.col0.z, scale.z * r.col1.z, scale.z * r.col2.z, 0 ),
		Vec4f( translation.x, translation.y, translation.z, 1 )
	);
    // clang-format on

    return matrix;
}

void Transform3DComponent::from_matrix( const Mat4& xform )
{
    // this is adapted from ImGuizmo::DecomposeMatrixToComponents

    scale.x = Vec3f( xform.col0.x, xform.col0.y, xform.col0.z ).length();
    scale.y = Vec3f( xform.col1.x, xform.col1.y, xform.col1.z ).length();
    scale.z = Vec3f( xform.col2.x, xform.col2.y, xform.col2.z ).length();

    Mat4 r = xform.ortho_normalize();

    Vec3f euler;
    euler.x  = atan2f( r.col1.z, r.col2.z );
    euler.y  = atan2f( -r.col0.z, sqrtf( r.col1.z * r.col1.z + r.col2.z * r.col2.z ) );
    euler.z  = atan2f( r.col0.y, r.col0.x );
    rotation = Quaternion( euler ); // i've no idea how to extract quaternions directly from transform matrix,
                                    // so we just convert euler to quaternion here...

    translation.x = r.col3.x;
    translation.y = r.col3.y;
    translation.z = r.col3.z;
}

static inline float col_length( const Vec4f& c )
{
    return std::sqrt( c.x * c.x + c.y * c.y + c.z * c.z );
}

Mat4 Transform3DComponent::parent_contrib( const Mat4& parent_global ) const
{
    Mat4 T = Mat4::identity();
    if (inherit_position)
        T.col3 = Vec4f( parent_global.col3.x, parent_global.col3.y, parent_global.col3.z, 1.0f );

    Mat4 R = Mat4::identity();
    if (inherit_rotation) {
        R      = Mat4::ortho_normalize( parent_global );
        R.col3 = Vec4f( 0, 0, 0, 1 );
    }

    Mat4 S = Mat4::identity();
    if (inherit_scale) {
        S.col0.x = col_length( parent_global.col0 );
        S.col1.y = col_length( parent_global.col1 );
        S.col2.z = col_length( parent_global.col2 );
    }

    return T * R * S;
}

void Constraint3DComponent::apply_position_locks( Transform3DComponent* xform ) const
{
    NC_FAIL_MSG_RET( xform, "Transform component is not given" );

    if (lock_position_x)
        xform->global.col3.x = xform->translation.x;
    if (lock_position_y)
        xform->global.col3.y = xform->translation.y;
    if (lock_position_z)
        xform->global.col3.z = xform->translation.z;
}

} // namespace nc
