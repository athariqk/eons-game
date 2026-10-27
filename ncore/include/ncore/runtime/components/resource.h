#pragma once

#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/resources/resource.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

NC_COMPONENT_API( HasResourceTag ){};

NC_COMPONENT_API( ResourceLoadedComponent )
{
    REFLECT RID resource_id;
    REFLECT ResourceFormatID format_id;
    RID replaced = 0; // previous handle when this notification came from a forced reload.
};

} // namespace nc
