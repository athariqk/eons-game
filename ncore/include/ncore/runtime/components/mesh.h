#pragma once

#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

NC_COMPONENT_API( MeshComponent )
{
    REFLECT RID source = 0;
};

NC_COMPONENT_API( MeshRenderComponent )
{
    REFLECT_HINT( rtti::PropertyFlags::HIDDEN ) RID item = 0; // spatial item RID.
    REFLECT uint32_t instance_count = 1;
    REFLECT uint16_t layer_index    = 0;
};

} // namespace nc
