#pragma once

#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/services/video/window/window_types.h>

namespace nc {

NC_COMPONENT_API( WindowComponent )
{
    REFLECT uint32_t Source = UINT32_MAX; // The window ID, from WindowService.
    REFLECT RID Swapchain;
    REFLECT StringView Title     = "NCORE Engine";
    REFLECT Vec2i Resolution     = Vec2i();
    REFLECT WindowMode Mode      = WindowMode::WINDOWED;
    REFLECT bool Visible         = false;
    REFLECT float PixelsPerMeter = 0;
};

NC_COMPONENT_API( WindowResizedComponent )
{
    REFLECT Vec2i NewSize;
};

NC_COMPONENT_API( MainWindowTag ){};

} // namespace nc
