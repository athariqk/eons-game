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
    REFLECT Vec3 Translation    = Vec3();                 // Local translation.
    REFLECT Quaternion Rotation = Quaternion::identity(); // Local rotation.
    REFLECT Vec3 Scale          = Vec3( 1, 1, 1 );        // Local scale.
    REFLECT Mat4 Global         = Mat4::identity();       // Global transform, auto-computed.

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
};

} // namespace nc
