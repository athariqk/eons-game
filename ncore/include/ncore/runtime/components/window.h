#pragma once

#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/services/video/window/window_types.h>

namespace nc {

struct NCAPI WindowComponent {
    uint32_t SourceId      = UINT32_MAX; // The window ID, from WindowService.
    std::string_view Title = "NCORE Engine";
    Vec2 Resolution        = Vec2();
    bool Fullscreen        = false;
    bool Visible           = false;
    bool VSync             = false;
    float PixelsPerMeter   = 0;

    NSTRUCTV(
        WindowComponent, NC_F( WindowComponent, SourceId ), NC_F( WindowComponent, Title ),
        NC_F( WindowComponent, Resolution ), NC_F( WindowComponent, Fullscreen ), NC_F( WindowComponent, Visible ),
        NC_F( WindowComponent, VSync ), NC_F( WindowComponent, PixelsPerMeter )
    )
};

struct NCAPI SwapChainComponent {
    RID Source = 0;
    Vec2 Size  = Vec2();
    bool vsync = false;

    NSTRUCTV(
        SwapChainComponent, NC_F( SwapChainComponent, Source ), NC_F( SwapChainComponent, Size ),
        NC_F( SwapChainComponent, vsync )
    )
};

struct NCAPI SwapChainResizedComponent {
    Vec2 size = Vec2();
    NSTRUCTV( SwapChainResizedComponent, NC_F( SwapChainResizedComponent, size ) )
};

struct NCAPI MainWindowTag {
    NSTRUCT1( MainWindowTag )
};

struct NCAPI GuiStateComponent {
    bool SubmitToGPU              = true;
    ImGuiContext* ImGuiCtx = nullptr;
    HashMap<ImGuiMouseCursor, CursorType> CursorMap;
    RID Material;
    RID LastTexId;
    NSTRUCTV(
        GuiStateComponent, NC_F( GuiStateComponent, SubmitToGPU ), NC_F( GuiStateComponent, ImGuiCtx ),
        NC_F( GuiStateComponent, CursorMap ), NC_F( GuiStateComponent, Material ), NC_F( GuiStateComponent, LastTexId )
    )
};

} // namespace nc
