#pragma once

#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/resources/resource.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

NC_COMPONENT_API( HasResourceTag )
{
};

NC_COMPONENT_API( ResourceLoadedComponent )
{
    REFLECT RID ResourceId;
    REFLECT ResourceFormatID format_id;
};

} // namespace nc
