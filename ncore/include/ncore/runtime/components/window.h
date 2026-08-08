#pragma once

#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/services/video/window/window_types.h>

namespace nc {

struct NCAPI WindowComponent {
    uint32_t id            = UINT32_MAX; // The window ID, from WindowService
    std::string_view title = "NCORE Engine";
    Vec2 resolution{};
    bool fullscreen        = false;
    bool visible           = false;
    bool vsync             = false;
    float pixels_per_meter = 0;

    NSTRUCT(
        WindowComponent, NC_F( WindowComponent, id ) NC_F( WindowComponent, title ) NC_F( WindowComponent, resolution )
                             NC_F( WindowComponent, fullscreen ) NC_F( WindowComponent, visible )
                                 NC_F( WindowComponent, vsync ) NC_F( WindowComponent, pixels_per_meter )
    )
};

struct NCAPI SwapChainComponent {
    RID swapchain{};
    Vec2 size{};
    bool vsync = false;

    NSTRUCT(
        SwapChainComponent,
        NC_F( SwapChainComponent, swapchain ) NC_F( SwapChainComponent, size ) NC_F( SwapChainComponent, vsync )
    )
};

struct NCAPI SwapChainResizedComponent {
    Vec2 size{};
    NSTRUCT( SwapChainResizedComponent, NC_F( SwapChainResizedComponent, size ) )
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"
struct NCAPI MainWindowTag {
    NSTRUCT( MainWindowTag )
};
#pragma clang diagnostic pop

struct NCAPI GuiStateComponent {
    ImGuiContext* imctx = nullptr;
    HashMap<ImGuiMouseCursor, CursorType> cursor_map;
    RID material;
    RID last_tex_id;
    NSTRUCT(
        GuiStateComponent,
        NC_F( GuiStateComponent, imctx ) NC_F( GuiStateComponent, cursor_map ) NC_F( GuiStateComponent, material )
    )
};

} // namespace nc
