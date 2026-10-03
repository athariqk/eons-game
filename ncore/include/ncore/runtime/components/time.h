#pragma once

#include <ncore.h>
#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

NC_COMPONENT_API( TimeComponent )
{
    REFLECT uint32_t Ticks     = 0;
    REFLECT int FrameCount     = 0;
    REFLECT double FPS         = 0;
    REFLECT double Accumulator = 0.0;
};

} // namespace nc
