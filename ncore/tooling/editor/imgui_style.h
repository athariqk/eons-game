#pragma once

#include <imgui.h>

namespace nc::editor {

// Claude generated, may need further tweaks

static void StyleColorsEditor()
{
    auto& colors = ImGui::GetStyle().Colors;

    // Base panels: flatter, cooler charcoal-navy than StyleColorsNcoreDark,
    // closer to Hammer's #3C3C3C but shifted cold
    colors[ImGuiCol_WindowBg]      = ImVec4( 0.06f, 0.07f, 0.09f, 1.00f );
    colors[ImGuiCol_ChildBg]       = ImVec4( 0.05f, 0.06f, 0.08f, 1.00f );
    colors[ImGuiCol_PopupBg]       = ImVec4( 0.04f, 0.05f, 0.07f, 0.98f );
    colors[ImGuiCol_MenuBarBg]     = ImVec4( 0.045f, 0.055f, 0.075f, 1.00f );
    colors[ImGuiCol_TitleBg]       = ImVec4( 0.03f, 0.04f, 0.055f, 1.00f );
    colors[ImGuiCol_TitleBgActive] = ImVec4( 0.05f, 0.07f, 0.10f, 1.00f );

    // Hairline borders everywhere — this is the Source "toolbox" read
    colors[ImGuiCol_Border]           = ImVec4( 0.16f, 0.22f, 0.27f, 0.90f );
    colors[ImGuiCol_Separator]        = ImVec4( 0.16f, 0.22f, 0.27f, 0.70f );
    colors[ImGuiCol_SeparatorHovered] = ImVec4( 0.25f, 0.75f, 0.90f, 0.80f ); // cyan glow on interaction

    // Flat frame widgets, no gradient feel
    colors[ImGuiCol_FrameBg]        = ImVec4( 0.09f, 0.11f, 0.14f, 1.00f );
    colors[ImGuiCol_FrameBgHovered] = ImVec4( 0.10f, 0.30f, 0.38f, 0.85f );
    colors[ImGuiCol_FrameBgActive]  = ImVec4( 0.12f, 0.42f, 0.52f, 1.00f );

    // Cyan accent = "this is live / selected / editable", SpaceEngine signature
    colors[ImGuiCol_CheckMark]        = ImVec4( 0.30f, 0.85f, 0.95f, 1.00f );
    colors[ImGuiCol_SliderGrab]       = ImVec4( 0.25f, 0.75f, 0.90f, 1.00f );
    colors[ImGuiCol_SliderGrabActive] = ImVec4( 0.40f, 0.90f, 1.00f, 1.00f );
    colors[ImGuiCol_Button]           = ImVec4( 0.09f, 0.13f, 0.16f, 1.00f );
    colors[ImGuiCol_ButtonHovered]    = ImVec4( 0.12f, 0.35f, 0.45f, 1.00f );
    colors[ImGuiCol_ButtonActive]     = ImVec4( 0.20f, 0.65f, 0.85f, 1.00f );
    colors[ImGuiCol_Header]           = ImVec4( 0.10f, 0.28f, 0.38f, 0.55f );
    colors[ImGuiCol_HeaderHovered]    = ImVec4( 0.15f, 0.45f, 0.60f, 0.75f );
    colors[ImGuiCol_HeaderActive]     = ImVec4( 0.20f, 0.65f, 0.85f, 0.90f );

    // Flat tabs, Source-style — no gradient, sharp on/off states
    colors[ImGuiCol_Tab]                 = ImVec4( 0.06f, 0.08f, 0.10f, 1.00f );
    colors[ImGuiCol_TabHovered]          = ImVec4( 0.12f, 0.35f, 0.45f, 0.90f );
    colors[ImGuiCol_TabSelected]         = ImVec4( 0.09f, 0.13f, 0.17f, 1.00f );
    colors[ImGuiCol_TabSelectedOverline] = ImVec4( 0.30f, 0.85f, 0.95f, 1.00f ); // cyan top-edge tab marker

    colors[ImGuiCol_Text]         = ImVec4( 0.85f, 0.92f, 0.95f, 1.00f );
    colors[ImGuiCol_TextDisabled] = ImVec4( 0.40f, 0.48f, 0.53f, 1.00f );
    colors[ImGuiCol_TextLink]     = ImVec4( 0.35f, 0.85f, 0.95f, 1.00f );

    colors[ImGuiCol_ScrollbarBg]   = ImVec4( 0.03f, 0.04f, 0.05f, 0.60f );
    colors[ImGuiCol_ScrollbarGrab] = ImVec4( 0.15f, 0.30f, 0.38f, 0.85f );

    colors[ImGuiCol_ResizeGrip]        = ImVec4( 0.20f, 0.55f, 0.68f, 0.35f );
    colors[ImGuiCol_ResizeGripHovered] = ImVec4( 0.30f, 0.80f, 0.95f, 0.65f );
    colors[ImGuiCol_ResizeGripActive]  = ImVec4( 0.40f, 0.90f, 1.00f, 0.90f );

    colors[ImGuiCol_NavCursor] = ImVec4( 0.35f, 0.85f, 0.95f, 1.00f );
}

static void StyleSizesEditor()
{
    auto& style = ImGui::GetStyle();

    // Source-tool density: tighter than default, but not cramped
    style.WindowPadding    = ImVec2( 6.0f, 6.0f );
    style.FramePadding     = ImVec2( 6.0f, 3.0f );
    style.ItemSpacing      = ImVec2( 6.0f, 4.0f );
    style.ItemInnerSpacing = ImVec2( 4.0f, 4.0f );
    style.IndentSpacing    = 14.0f;
    style.ScrollbarSize    = 11.0f;
    style.GrabMinSize      = 8.0f;

    // Near-zero rounding = Source's flat tool aesthetic, not SpaceEngine's softness
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 2.0f; // just enough to not feel like Win95
    style.PopupRounding     = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 0.0f; // flat tabs, Source-style

    // Hairline borders — the defining "toolbox" cue
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize  = 1.0f;
    style.FrameBorderSize  = 1.0f;
    style.PopupBorderSize  = 1.0f;
    style.TabBorderSize    = 1.0f;

    style.FontSizeBase = 16.0f; // slightly smaller than your 18 — denser property panels
}

} // namespace nc::editor
