#pragma once

#include <ncore/core/matrix.h>
#include <ncore/core/quaternion.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>

namespace nc {

// TODO: turn into a 2x3 matrix
struct NCAPI EcsTransform2D {
    Vec2 position;
    Vec2 size;
    float angle = 0.0f;

    Vec2 get_world_center_point()
    {
        return Vec2( position.x + ( size.x * 0.5f ), position.y + ( size.y * 0.5f ) );
    }

    NSTRUCT(
        EcsTransform2D, NC_F( EcsTransform2D, position ) NC_F( EcsTransform2D, size ) NC_F( EcsTransform2D, angle )
    )
};

struct NCAPI EcsTransform3D {
    Vec3 translation    = Vec3();
    Quaternion rotation = Quaternion::identity();
    Vec3 scale          = Vec3( 1, 1, 1 );

    /**
     * @brief Builds 4x4 transform/model matrix from translation, rotation and scale.
     */
    Mat4 get_matrix() const;

    NSTRUCT(
        EcsTransform3D,
        NC_F( EcsTransform3D, translation ) NC_F( EcsTransform3D, rotation ) NC_F( EcsTransform3D, scale )
    )
};

} // namespace nc
