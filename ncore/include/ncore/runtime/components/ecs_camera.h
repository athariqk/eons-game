#pragma once

#include <ncore/core/vector.h>

namespace nc {

struct NCAPI EcsCamera {
    enum Mode : uint8_t {
        ORTHOGRAPHIC,
        PERSPECTIVE
    };

    Mode mode = ORTHOGRAPHIC;
    Vec2 ortho_size{ 1280, 720 };
    float fov        = 60.0f;
    float z_near = 0.1f;
    float z_far  = 1000.0f;

    NSTRUCT(
        EcsCamera, NC_F( EcsCamera, mode ) NC_F( EcsCamera, ortho_size ) NC_F( EcsCamera, fov )
                       NC_F( EcsCamera, z_near ) NC_F( EcsCamera, z_far )
    )
};

} // namespace nc
