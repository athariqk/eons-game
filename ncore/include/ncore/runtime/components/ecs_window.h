#pragma once

#include <ncore/kernel/rid.h>
#include <ncore/kernel/structures.h>
#include <ncore/kernel/types.h>

namespace nc {

struct NCORE_API EcsWindow {
    uint32_t id = UINT32_MAX; // The window ID, from VideoModule
    Vec2 resolution{};
    bool fullscreen        = false;
    bool visible           = false;
    bool vsync             = false;
    float pixels_per_meter = 0;

    NSTRUCT(
        EcsWindow, NC_F( EcsWindow, id ) NC_F( EcsWindow, resolution ) NC_F( EcsWindow, fullscreen )
                       NC_F( EcsWindow, visible ) NC_F( EcsWindow, vsync ) NC_F( EcsWindow, pixels_per_meter )
    )
};

struct NCORE_API EcsTargetSurface {
    RID surface{};
    bool vsync = false;

    NSTRUCT( EcsTargetSurface, NC_F( EcsTargetSurface, surface ) NC_F( EcsTargetSurface, vsync ) )
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"
struct NCORE_API EcsMainWindow {
    NSTRUCT( EcsMainWindow )
};
#pragma clang diagnostic pop

} // namespace nc
