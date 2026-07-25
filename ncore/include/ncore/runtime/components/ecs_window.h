#pragma once

#include <ncore/core/rid.h>
#include <ncore/core/structures.h>
#include <ncore/core/types.h>

namespace nc {

struct NCAPI EcsWindow {
    uint32_t id            = UINT32_MAX; // The window ID, from WindowModule
    std::string_view title = "NCORE Engine";
    Vec2 resolution{};
    bool fullscreen        = false;
    bool visible           = false;
    bool vsync             = false;
    float pixels_per_meter = 0;

    NSTRUCT(
        EcsWindow,
        NC_F( EcsWindow, id ) NC_F( EcsWindow, title ) NC_F( EcsWindow, resolution ) NC_F( EcsWindow, fullscreen )
            NC_F( EcsWindow, visible ) NC_F( EcsWindow, vsync ) NC_F( EcsWindow, pixels_per_meter )
    )
};

struct NCAPI EcsSwapChainRef {
    RID swap_chain{};
    Vec2 size{};
    bool vsync = false;

    NSTRUCT(
        EcsSwapChainRef,
        NC_F( EcsSwapChainRef, swap_chain ) NC_F( EcsSwapChainRef, size ) NC_F( EcsSwapChainRef, vsync )
    )
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"
struct NCAPI EcsMainWindow {
    NSTRUCT( EcsMainWindow )
};
#pragma clang diagnostic pop

} // namespace nc
