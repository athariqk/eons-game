#include <ncore/core/matrix.h>
#include <ncore/runtime/components/transform.h>

namespace nc {

Mat4 nc::Transform3DComponent::to_matrix() const
{
    Mat3 r = Rotation.to_rotation_matrix();

    // clang-format off
	// TransformMatrix = Translation * Rotation * Scale
	Mat4 matrix = Mat4(
		Vec4( Scale.x * r.col0.x, Scale.x * r.col1.x, Scale.x * r.col2.x, 0 ),
		Vec4( Scale.y * r.col0.y, Scale.y * r.col1.y, Scale.y * r.col2.y, 0 ),
		Vec4( Scale.z * r.col0.z, Scale.z * r.col1.z, Scale.z * r.col2.z, 0 ),
		Vec4( Translation.x, Translation.y, Translation.z, 1 )
	);
    // clang-format on

    return matrix;
}

void Transform3DComponent::from_matrix( const Mat4& xform )
{
    // this is adapted from ImGuizmo::DecomposeMatrixToComponents

    Scale.x = Vec3( xform.col0.x, xform.col0.y, xform.col0.z ).length();
    Scale.y = Vec3( xform.col1.x, xform.col1.y, xform.col1.z ).length();
    Scale.z = Vec3( xform.col2.x, xform.col2.y, xform.col2.z ).length();

    Mat4 r = xform.ortho_normalize();

    Vec3 euler;
    euler.x  = atan2f( r.col1.z, r.col2.z );
    euler.y  = atan2f( -r.col0.z, sqrtf( r.col1.z * r.col1.z + r.col2.z * r.col2.z ) );
    euler.z  = atan2f( r.col0.y, r.col0.x );
    Rotation = Quaternion( euler ); // i've no idea how to extract quaternions directly from transform matrix,
                                    // so we just convert euler to quaternion here...

    Translation.x = r.col3.x;
    Translation.y = r.col3.y;
    Translation.z = r.col3.z;
}

} // namespace nc
