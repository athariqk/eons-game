#pragma once

#include <ncore/core/color.h>
#include <ncore/core/rid.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

NC_COMPONENT_API( SpriteComponent )
{
    REFLECT RID source;
    REFLECT RID texture;
    REFLECT Color Tint{ 255, 255, 255, 255 };
};

NC_COMPONENT( EcsCircleDraw )
{
    float radius = 1.0f;
    Color color{ 0, 0, 0, 255 };
    bool filled = false;
    bool edge   = false;
};

} // namespace nc
