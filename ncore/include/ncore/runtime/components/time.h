#pragma once

#include <ncore.h>
#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

NC_COMPONENT_API( TimeComponent )
{
    REFLECT uint32_t ticks     = 0;
    REFLECT int frame_count     = 0;
    REFLECT double fps         = 0;
    REFLECT double accumulator = 0.0;
};

} // namespace nc
