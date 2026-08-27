#pragma once

#include <ncore/core/types.h>
#include <ncore/core/vector.h>

namespace nc {

struct NCAPI WindowComponent {
    uint32_t Source = UINT32_MAX; // The window ID, from WindowService.
    RID Swapchain;
    std::string_view Title = "NCORE Engine";
    Vec2i Resolution       = Vec2i();
    bool Fullscreen        = false;
    bool Visible           = false;
    float PixelsPerMeter   = 0;
    NSTRUCTV(
        WindowComponent, NC_F( WindowComponent, Source ), NC_F( WindowComponent, Title ),
        NC_F( WindowComponent, Resolution ), NC_F( WindowComponent, Fullscreen ), NC_F( WindowComponent, Visible ),
        NC_F( WindowComponent, PixelsPerMeter )
    )
};

struct NCAPI WindowResizedComponent {
    Vec2i NewSize;
    NSTRUCTV( WindowResizedComponent, NC_F( WindowResizedComponent, NewSize ) )
};

struct NCAPI MainWindowTag {
    NSTRUCT1( MainWindowTag )
};

} // namespace nc
