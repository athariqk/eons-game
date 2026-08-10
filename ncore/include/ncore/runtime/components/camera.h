#pragma once

namespace nc {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"
struct NCAPI ActiveCameraTag {
    NSTRUCT( ActiveCameraTag )
};
#pragma clang diagnostic pop

/**
 * @brief A perspective camera.
 */
struct NCAPI CameraComponent {
    float fov       = 1.5708f; // In radians. Default is 90 degrees.
    float z_near    = 0.1f;
    float z_far     = 100.0f;
    bool mouse_locked = false;

    NSTRUCT(
        CameraComponent, NC_F( CameraComponent, fov ) NC_F( CameraComponent, z_near ) NC_F( CameraComponent, z_far )
                             NC_F( CameraComponent, mouse_locked )
    )
};

} // namespace nc
