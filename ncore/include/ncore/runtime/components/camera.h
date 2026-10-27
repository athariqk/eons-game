#pragma once

#include <ncore/core/rect.h>
#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

/**
 * @brief A 3D/2D scene camera.
 */
NC_COMPONENT_API( CameraComponent )
{
    REFLECT_HINT( rtti::PropertyFlags::HIDDEN ) RID source = 0;
    /**
     * @brief In radians. Default is 90 degrees.
     */
    REFLECT float field_of_view = 1.5708f;
    REFLECT float z_near       = 0.1f;
    REFLECT float z_far        = 1000.0f;
    /**
     * @brief Whether the camera's mouse is captured.
     */
    REFLECT bool mouse_captured = false;
    /**
     * @brief Whether the camera uses perspective projection.
     */
    REFLECT bool perspective = true;
    /**
     * @brief Color texture for the camera's render target.
     */
    REFLECT_HINT( rtti::PropertyFlags::HIDDEN ) RID render_texture = 0;
    /**
     * @brief Depth texture for the camera's render target.
     */
    REFLECT_HINT( rtti::PropertyFlags::HIDDEN ) RID depth_texture = 0;
    /**
     * @brief The rectangle on the screen where this camera will render to.
     */
    REFLECT Rect2i display_rect = Rect2i();
    /**
     * @brief If true, the camera will render to the primary swapchain.
     * If false, it will render to its own renderable textures.
     */
    REFLECT bool render_to_screen = true;
    /**
     * @brief Whether to draw the camera's canvas.
     */
    REFLECT bool draw_canvas = true;
};

NC_COMPONENT_API( ActiveCameraTag ){};

} // namespace nc
