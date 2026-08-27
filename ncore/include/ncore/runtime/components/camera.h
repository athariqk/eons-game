#pragma once

#include <ncore/core/rect.h>
#include <ncore/core/rid.h>

namespace nc {

/**
 * @brief A camera.
 */
struct NCAPI CameraComponent {
    RID Source;
    float FieldOfView  = 1.5708f; // In radians. Default is 90 degrees.
    float zNear        = 0.1f;
    float zFar         = 100.0f;
    bool MouseCaptured = false;
    bool Perspective   = true;
    RID RenderTexture;
    RID DepthTexture;
    Rect2i DisplayRect;
    bool RenderToScreen = true;
    bool DrawCanvas     = true;

    NSTRUCTV(
        CameraComponent, NC_F( CameraComponent, Source ), NC_F( CameraComponent, FieldOfView ),
        NC_F( CameraComponent, zNear ), NC_F( CameraComponent, zFar ), NC_F( CameraComponent, MouseCaptured ),
        NC_F( CameraComponent, Perspective ), NC_F( CameraComponent, RenderTexture ),
        NC_F( CameraComponent, DepthTexture ), NC_F( CameraComponent, DisplayRect ),
        NC_F( CameraComponent, RenderToScreen ), NC_F( CameraComponent, DrawCanvas )
    )
};

struct NCAPI ActiveCameraTag {
    NSTRUCT1( ActiveCameraTag )
};

} // namespace nc
