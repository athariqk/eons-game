#pragma once

#include <ncore/core/matrix.h>
#include <ncore/core/quaternion.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

// TODO: turn into a 2x3 matrix
NC_COMPONENT_API( Transform2DComponent )
{
    REFLECT Vec2f position;
    REFLECT Vec2f Size;
    REFLECT float Angle = 0.0f;

    Vec2f get_world_center_point()
    {
        return Vec2f( position.x + ( Size.x * 0.5f ), position.y + ( Size.y * 0.5f ) );
    }
};

NC_COMPONENT_API( Transform3DComponent )
{
    /**
     * @brief Local translation.
     */
    REFLECT Vec3f Translation = Vec3f();
    /**
     * @brief Local rotation.
     */
    REFLECT Quaternion Rotation = Quaternion::identity();
    /**
     * @brief Local scale.
     */
    REFLECT Vec3f Scale = Vec3f( 1, 1, 1 );
    /**
     * @brief Global transform, auto-computed.
     */
    REFLECT Mat4 Global = Mat4::identity();

    REFLECT bool InheritPosition = true;
    REFLECT bool InheritRotation = true;
    REFLECT bool InheritScale    = true;

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

NC_COMPONENT_API( Transform3DConstraintsComponent )
{
    REFLECT bool LockPositionX = false;
    REFLECT bool LockPositionY = false;
    REFLECT bool LockPositionZ = false;

    void apply_position_locks( Transform3DComponent * xform ) const;
};

} // namespace nc
