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
    float near_plane = 0.1f;
    float far_plane  = 1000.0f;
    Vec4 viewport{ 0, 0, 1, 1 };

    NSTRUCT(
        EcsCamera, NC_F( EcsCamera, mode ) NC_F( EcsCamera, ortho_size ) NC_F( EcsCamera, fov )
                       NC_F( EcsCamera, near_plane ) NC_F( EcsCamera, far_plane ) NC_F( EcsCamera, viewport )
    )
};

} // namespace nc
