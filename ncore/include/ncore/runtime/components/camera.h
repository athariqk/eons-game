#pragma once

namespace nc {

struct NCAPI ActiveCameraTag {
    NSTRUCT1( ActiveCameraTag )
};

/**
 * @brief A perspective camera.
 */
struct NCAPI CameraComponent {
    float FieldOfView  = 1.5708f; // In radians. Default is 90 degrees.
    float zNear        = 0.1f;
    float zFar         = 100.0f;
    bool MouseCaptured = false;

    NSTRUCTV(
        CameraComponent, NC_F( CameraComponent, FieldOfView ), NC_F( CameraComponent, zNear ),
        NC_F( CameraComponent, zFar ), NC_F( CameraComponent, MouseCaptured )
    )
};

} // namespace nc
