#pragma once

#include <ncore/core/rect.h>
#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

/**
 * @brief A camera.
 */
NC_COMPONENT_API( CameraComponent )
{
    REFLECT RID Source;
    REFLECT float FieldOfView  = 1.5708f; // In radians. Default is 90 degrees.
    REFLECT float zNear        = 0.1f;
    REFLECT float zFar         = 1000.0f;
    REFLECT bool MouseCaptured = false;
    REFLECT bool Perspective   = true;
    REFLECT RID RenderTexture;
    REFLECT RID DepthTexture;
    REFLECT Rect2i DisplayRect;
    REFLECT bool RenderToScreen = true;
    REFLECT bool DrawCanvas     = true;
};

NC_COMPONENT_API( ActiveCameraTag ){};

} // namespace nc
