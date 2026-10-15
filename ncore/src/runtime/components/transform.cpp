#include <ncore/core/matrix.h>
#include <ncore/core/vector.h>
#include <ncore/runtime/components/transform.h>

namespace nc {

Mat4 nc::Transform3DComponent::to_matrix() const
{
    Mat3 r = Rotation.to_rotation_matrix();

    // clang-format off
	// TransformMatrix = Translation * Rotation * Scale
	Mat4 matrix = Mat4(
		Vec4f( Scale.x * r.col0.x, Scale.x * r.col1.x, Scale.x * r.col2.x, 0 ),
		Vec4f( Scale.y * r.col0.y, Scale.y * r.col1.y, Scale.y * r.col2.y, 0 ),
		Vec4f( Scale.z * r.col0.z, Scale.z * r.col1.z, Scale.z * r.col2.z, 0 ),
		Vec4f( Translation.x, Translation.y, Translation.z, 1 )
	);
    // clang-format on

    return matrix;
}

void Transform3DComponent::from_matrix( const Mat4& xform )
{
    // this is adapted from ImGuizmo::DecomposeMatrixToComponents

    Scale.x = Vec3f( xform.col0.x, xform.col0.y, xform.col0.z ).length();
    Scale.y = Vec3f( xform.col1.x, xform.col1.y, xform.col1.z ).length();
    Scale.z = Vec3f( xform.col2.x, xform.col2.y, xform.col2.z ).length();

    Mat4 r = xform.ortho_normalize();

    Vec3f euler;
    euler.x  = atan2f( r.col1.z, r.col2.z );
    euler.y  = atan2f( -r.col0.z, sqrtf( r.col1.z * r.col1.z + r.col2.z * r.col2.z ) );
    euler.z  = atan2f( r.col0.y, r.col0.x );
    Rotation = Quaternion( euler ); // i've no idea how to extract quaternions directly from transform matrix,
                                    // so we just convert euler to quaternion here...

    Translation.x = r.col3.x;
    Translation.y = r.col3.y;
    Translation.z = r.col3.z;
}

static inline float col_length( const Vec4f& c )
{
    return std::sqrt( c.x * c.x + c.y * c.y + c.z * c.z );
}

Mat4 Transform3DComponent::parent_contrib( const Mat4& parent_global ) const
{
    Mat4 T = Mat4::identity();
    if (InheritPosition)
        T.col3 = Vec4f( parent_global.col3.x, parent_global.col3.y, parent_global.col3.z, 1.0f );

    Mat4 R = Mat4::identity();
    if (InheritRotation) {
        R      = Mat4::ortho_normalize( parent_global );
        R.col3 = Vec4f( 0, 0, 0, 1 );
    }

    Mat4 S = Mat4::identity();
    if (InheritScale) {
        S.col0.x = col_length( parent_global.col0 );
        S.col1.y = col_length( parent_global.col1 );
        S.col2.z = col_length( parent_global.col2 );
    }

    return T * R * S;
}

void Transform3DConstraintsComponent::apply_position_locks( Transform3DComponent* xform ) const
{
    NC_FAIL_MSG_RET( xform, "Transform component is not given" );

    if (LockPositionX)
        xform->Global.col3.x = xform->Translation.x;
    if (LockPositionY)
        xform->Global.col3.y = xform->Translation.y;
    if (LockPositionZ)
        xform->Global.col3.z = xform->Translation.z;
}

} // namespace nc
