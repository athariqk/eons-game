#pragma once

#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/services/video/window/window_types.h>

namespace nc {

NC_COMPONENT_API( WindowComponent )
{
    REFLECT_HINT( rtti::PropertyFlags::HIDDEN ) uint32_t source = UINT32_MAX; // The window ID, from WindowService.
    REFLECT_HINT( rtti::PropertyFlags::HIDDEN ) RID swapchain;
    REFLECT StringView title       = "NCORE Engine";
    REFLECT Vec2i resolution       = Vec2i();
    REFLECT WindowMode mode        = WindowMode::WINDOWED;
    REFLECT bool visible           = false;
    REFLECT float pixels_per_meter = 0;
};

NC_COMPONENT_API( WindowResizedComponent )
{
    REFLECT Vec2i new_size;
};

NC_COMPONENT_API( MainWindowTag ){};

} // namespace nc
