#pragma once

#include <ncore/core/vector.h>

namespace nc {

/**
 * @brief A perspective camera.
 */
struct NCAPI EcsCamera {
    float fov    = 1.5708f; // In radians. Default is 90 degrees.
    float z_near = 0.1f;
    float z_far  = 1000.0f;

    NSTRUCT( EcsCamera, NC_F( EcsCamera, fov ) NC_F( EcsCamera, z_near ) NC_F( EcsCamera, z_far ) )
};

} // namespace nc
