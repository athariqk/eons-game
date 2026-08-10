#pragma once

#include <ncore/core/matrix.h>
#include <ncore/core/quaternion.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>

namespace nc {

// TODO: turn into a 2x3 matrix
struct NCAPI Transform2DComponent {
    Vec2 position;
    Vec2 size;
    float angle = 0.0f;

    Vec2 get_world_center_point()
    {
        return Vec2( position.x + ( size.x * 0.5f ), position.y + ( size.y * 0.5f ) );
    }

    NSTRUCT(
        Transform2DComponent,
        NC_F( Transform2DComponent, position ) NC_F( Transform2DComponent, size ) NC_F( Transform2DComponent, angle )
    )
};

struct NCAPI Transform3DComponent {
    Vec3 translation    = Vec3();                 // Local translation.
    Quaternion rotation = Quaternion::identity(); // Local rotation.
    Vec3 scale          = Vec3( 1, 1, 1 );        // Local scale.
    Mat4 world          = Mat4::identity();       // Global transform, auto-computed.

    /**
     * @brief Builds 4x4 transform/model matrix from the local translation, rotation and scale.
     */
    Mat4 get_matrix() const;

    NSTRUCT(
        Transform3DComponent, NC_F( Transform3DComponent, translation ) NC_F( Transform3DComponent, rotation )
                                  NC_F( Transform3DComponent, scale ) NC_F( Transform3DComponent, world )
    )
};

} // namespace nc
