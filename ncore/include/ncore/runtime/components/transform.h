#pragma once

#include <ncore/core/matrix.h>
#include <ncore/core/quaternion.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>

namespace nc {

// TODO: turn into a 2x3 matrix
struct NCAPI Transform2DComponent {
    Vec2 position;
    Vec2 Size;
    float Angle = 0.0f;

    Vec2 get_world_center_point()
    {
        return Vec2( position.x + ( Size.x * 0.5f ), position.y + ( Size.y * 0.5f ) );
    }

    NSTRUCTV(
        Transform2DComponent,
        NC_F( Transform2DComponent, position ) NC_F( Transform2DComponent, Size ) NC_F( Transform2DComponent, Angle )
    )
};

struct NCAPI Transform3DComponent {
    Vec3 Translation    = Vec3();                 // Local translation.
    Quaternion Rotation = Quaternion::identity(); // Local rotation.
    Vec3 Scale          = Vec3( 1, 1, 1 );        // Local scale.
    Mat4 Global         = Mat4::identity();       // Global transform, auto-computed.

    /**
     * @brief Compose 4x4 transform/model matrix from
     * the local translation, rotation and scale.
     */
    Mat4 to_matrix() const;
    /**
     * @brief Decompose 4x4 transform/model matrix to
     * local translation, rotation and scale.
     */
    void from_matrix( const Mat4& xform );

    NSTRUCTV(
        Transform3DComponent, NC_F( Transform3DComponent, Translation ) NC_F( Transform3DComponent, Rotation )
                                  NC_F( Transform3DComponent, Scale ) NC_F( Transform3DComponent, Global )
    )
};

} // namespace nc
