#pragma once

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

} // namespace nc
