#pragma once

#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

NC_COMPONENT_API( InputComponent )
{
    REFLECT Vec3f direction         = Vec3f(); // normalized deltas of horizontal and vertical axis.
    REFLECT float magnitude         = 100.0f;  // scales direction - units/s.
    REFLECT Vec3f angular_delta     = Vec3f(); // yaw, pitch, and roll deltas in degrees.
    REFLECT float roll_rate         = 65.0f;   // deg/s.
    REFLECT float mouse_sensitivity = 0.15f;   // deg/px.
};

} // namespace nc
