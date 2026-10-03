#pragma once

#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

NC_COMPONENT_API( InputComponent )
{
    REFLECT Vec3 Direction         = Vec3(); // normalized deltas of horizontal and vertical axis.
    REFLECT float Magnitude        = 5.0f;   // scales direction - units/s.
    REFLECT Vec3 AngularDelta      = Vec3(); // yaw, pitch, and roll deltas in degrees.
    REFLECT float RollRate         = 65.0f;  // deg/s.
    REFLECT float MouseSensitivity = 0.15f;  // deg/px.
};

} // namespace nc
