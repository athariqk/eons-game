#pragma once

#include <ncore/core/vector.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

NC_COMPONENT_API( RigidbodyComponent )
{
    REFLECT Vec2f velocity;
    REFLECT Vec2f pending_force{ 0.0f, 0.0f };
    REFLECT Vec2f pending_impulse{ 0.0f, 0.0f };
    REFLECT float linear_damping = 2.5f;
};

} // namespace nc
