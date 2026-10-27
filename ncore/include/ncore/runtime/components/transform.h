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
    REFLECT_HINT( rtti::PropertyFlags::READ_ONLY ) Mat4 global = Mat4::identity();

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

} // namespace nc
