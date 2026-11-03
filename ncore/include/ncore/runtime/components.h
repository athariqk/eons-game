#pragma once

#include <ncore/core/collection.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/runtime/io/resource_loader.h>
#include <ncore/runtime/resources/image.h>
#include <ncore/runtime/resources/material.h>
#include <ncore/runtime/resources/mesh.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/video/render_service.h>
#include <ncore/services/video/window/window_types.h>
#include <ncore/services/video/window_service.h>
#include <ncore/structs/color.h>
#include <ncore/structs/matrix.h>
#include <ncore/structs/quaternion.h>
#include <ncore/structs/rect.h>
#include <ncore/structs/rid.h>
#include <ncore/types/types.h>
#include <ncore/structs/vector.h>

namespace nc {

/**
 * @brief A 3D/2D scene camera.
 */
NC_COMPONENT_API( CameraComponent )
{
    /**
     * @brief In radians. Default is 90 degrees.
     */
    REFLECT float field_of_view = 1.5708f;
    REFLECT float z_near        = 0.1f;
    REFLECT float z_far         = 1000.0f;
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
    REFLECT_HINT( types::PropertyFlags::HIDDEN ) RID render_texture = 0;
    /**
     * @brief Depth texture for the camera's render target.
     */
    REFLECT_HINT( types::PropertyFlags::HIDDEN ) RID depth_texture = 0;
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

    REFLECT_HINT( types::PropertyFlags::HIDDEN ) RID source = 0;
};

NC_COMPONENT_API( ActiveCameraTag ){};

// ----------------------------------------------------------------------------------

NC_COMPONENT_API( InputComponent )
{
    REFLECT Vec3f direction         = Vec3f(); // normalized deltas of horizontal and vertical axis.
    REFLECT float magnitude         = 100.0f;  // scales direction - units/s.
    REFLECT Vec3f angular_delta     = Vec3f(); // yaw, pitch, and roll deltas in degrees.
    REFLECT float roll_rate         = 65.0f;   // deg/s.
    REFLECT float mouse_sensitivity = 0.15f;   // deg/px.
};

// ----------------------------------------------------------------------------------

/**
 * @brief MeshInstanceComponent contains reference to a mesh plus the entity-specific
 * draw parameters.
 */
NC_COMPONENT_API( MeshInstanceComponent )
{
    REFLECT Ref<Mesh> mesh;
    REFLECT uint8_t layer_index     = 0;
    REFLECT uint32_t instance_count = 1;

    /**
     * @brief The internal spatial item RID, populated by the engine. DO NOT TOUCH.
     */
    REFLECT_HINT( types::PropertyFlags::HIDDEN ) RID internal_handle = 0;
};

// ----------------------------------------------------------------------------------

/**
 * @brief Material override for an entity's mesh.
 *
 * Without `override_mesh_material`, the mesh's own material is drawn.
 */
NC_COMPONENT_API( MaterialOverrideComponent )
{
    REFLECT Ref<Material> material;

    /**
     * @brief If true, draw with this material instead of the mesh's own.
     */
    REFLECT bool override_mesh_material = false;
};

// ----------------------------------------------------------------------------------

NC_COMPONENT_API( RigidbodyComponent )
{
    REFLECT Vec2f velocity;
    REFLECT Vec2f pending_force{ 0.0f, 0.0f };
    REFLECT Vec2f pending_impulse{ 0.0f, 0.0f };
    REFLECT float linear_damping = 2.5f;
};

// ----------------------------------------------------------------------------------

/**
 * @brief Convenience component containing video-related
 * services resolved from ServiceDB.
 *
 * May be used from ECS systems to interact with the engine.
 */
NC_COMPONENT_API( VideoServices )
{
    REFLECT WindowService* window   = nullptr;
    REFLECT RenderService* renderer = nullptr;
};

/**
 * @brief Convenience component containing input/output-related
 * services resolved from ServiceDB.
 *
 * May be used from ECS systems to interact with the engine.
 */
NC_COMPONENT_API( IoServices )
{
    REFLECT ResourceLoader* resources = nullptr;
    REFLECT InputService* inputs      = nullptr;
};

// ----------------------------------------------------------------------------------

NC_COMPONENT_API( SpriteComponent )
{
    REFLECT Ref<Image> texture;
    REFLECT Color tint{ 255, 255, 255, 255 };
};

NC_COMPONENT( EcsCircleDraw )
{
    float radius = 1.0f;
    Color color{ 0, 0, 0, 255 };
    bool filled = false;
    bool edge   = false;
};

// ----------------------------------------------------------------------------------

NC_COMPONENT_API( TimeComponent )
{
    REFLECT uint32_t ticks     = 0;
    REFLECT int frame_count    = 0;
    REFLECT double fps         = 0;
    REFLECT double accumulator = 0.0;
};

// ----------------------------------------------------------------------------------

// TODO: turn into a 2x3 matrix
NC_COMPONENT_API( Transform2DComponent )
{
    REFLECT Vec2f position;
    REFLECT Vec2f size;
    REFLECT float angle = 0.0f;

    Vec2f get_world_center_point()
    {
        return Vec2f( position.x + ( size.x * 0.5f ), position.y + ( size.y * 0.5f ) );
    }
};

NC_COMPONENT_API( Transform3DComponent )
{
    /**
     * @brief Local translation.
     */
    REFLECT Vec3f translation = Vec3f();
    /**
     * @brief Local rotation.
     */
    REFLECT Quaternion rotation = Quaternion::identity();
    /**
     * @brief Local scale.
     */
    REFLECT Vec3f scale = Vec3f( 1, 1, 1 );
    /**
     * @brief Global transform, auto-computed.
     */
    REFLECT_HINT( types::PropertyFlags::READ_ONLY ) Mat4 global = Mat4::identity();

    REFLECT bool inherit_position = true;
    REFLECT bool inherit_rotation = true;
    REFLECT bool inherit_scale    = true;

    /**
     * @brief Compose 4x4 transform/model matrix from
     * the local translation, rotation and scale.
     */
    Mat4 to_matrix() const;
    /**
     * @brief Decompose 4x4 transform/model matrix to
     * local translation, rotation and scale.
     */
    void from_matrix( const Mat4& xform );

    Mat4 parent_contrib( const Mat4& parent_global ) const;
};

NC_COMPONENT_API( Constraint3DComponent )
{
    REFLECT bool lock_position_x = false;
    REFLECT bool lock_position_y = false;
    REFLECT bool lock_position_z = false;

    void apply_position_locks( Transform3DComponent * xform ) const;
};

// ----------------------------------------------------------------------------------

NC_COMPONENT_API( WindowComponent )
{
    REFLECT_HINT( types::PropertyFlags::HIDDEN ) uint32_t source = UINT32_MAX; // The window ID, from WindowService.
    REFLECT_HINT( types::PropertyFlags::HIDDEN ) RID swapchain;
    REFLECT StringView title       = "NCORE Engine";
    REFLECT Vec2i resolution       = Vec2i();
    REFLECT WindowMode mode        = WindowMode::WINDOWED;
    REFLECT bool visible           = false;
    REFLECT float pixels_per_meter = 0;
};

NC_COMPONENT_API( WindowResizedComponent )
{
    REFLECT Vec2i new_size;
};

NC_COMPONENT_API( MainWindowTag ){};

} // namespace nc
