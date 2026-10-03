#pragma once

#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

NC_COMPONENT_API( MeshComponent )
{
    REFLECT RID Source = 0;
};

NC_COMPONENT_API( MeshRenderComponent )
{
    REFLECT RID Item           = 0; // spatial item RID (spatial_item_create)
    REFLECT uint32_t InstanceCount = 1;
};

} // namespace nc
