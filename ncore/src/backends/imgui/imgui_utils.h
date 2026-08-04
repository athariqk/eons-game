#pragma once

#include <imgui.h>

#include <ncore/modules/input/input_event.h>
#include <ncore/modules/video/window/window_types.h>

namespace nc {

static void StyleColorsNcore()
{
    auto& colors                               = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                      = ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );
    colors[ImGuiCol_TextDisabled]              = ImVec4( 0.71f, 0.71f, 0.71f, 1.00f );
    colors[ImGuiCol_WindowBg]                  = ImVec4( 0.21f, 0.43f, 0.54f, 0.78f );
    colors[ImGuiCol_ChildBg]                   = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
    colors[ImGuiCol_PopupBg]                   = ImVec4( 0.08f, 0.11f, 0.14f, 0.97f );
    colors[ImGuiCol_Border]                    = ImVec4( 1.00f, 1.00f, 1.00f, 0.71f );
    colors[ImGuiCol_BorderShadow]              = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
    colors[ImGuiCol_FrameBg]                   = ImVec4( 0.40f, 0.54f, 0.69f, 0.73f );
    colors[ImGuiCol_FrameBgHovered]            = ImVec4( 0.51f, 0.77f, 0.99f, 0.63f );
    colors[ImGuiCol_FrameBgActive]             = ImVec4( 0.51f, 0.77f, 0.99f, 0.82f );
    colors[ImGuiCol_TitleBg]                   = ImVec4( 0.10f, 0.20f, 0.28f, 1.00f );
    colors[ImGuiCol_TitleBgActive]             = ImVec4( 0.16f, 0.29f, 0.43f, 1.00f );
    colors[ImGuiCol_TitleBgCollapsed]          = ImVec4( 0.00f, 0.00f, 0.00f, 0.71f );
    colors[ImGuiCol_MenuBarBg]                 = ImVec4( 0.07f, 0.14f, 0.18f, 1.00f );
    colors[ImGuiCol_ScrollbarBg]               = ImVec4( 0.00f, 0.00f, 0.00f, 0.23f );
    colors[ImGuiCol_ScrollbarGrab]             = ImVec4( 0.56f, 0.56f, 0.56f, 1.00f );
    colors[ImGuiCol_ScrollbarGrabHovered]      = ImVec4( 0.64f, 0.64f, 0.64f, 1.00f );
    colors[ImGuiCol_ScrollbarGrabActive]       = ImVec4( 0.71f, 0.71f, 0.71f, 1.00f );
    colors[ImGuiCol_CheckMark]                 = ImVec4( 0.51f, 0.77f, 0.99f, 1.00f );
    colors[ImGuiCol_CheckboxSelectedBg]        = ImVec4( 0.47f, 0.70f, 0.90f, 0.67f );
    colors[ImGuiCol_SliderGrab]                = ImVec4( 0.49f, 0.72f, 0.94f, 1.00f );
    colors[ImGuiCol_SliderGrabActive]          = ImVec4( 0.51f, 0.77f, 0.99f, 1.00f );
    colors[ImGuiCol_Button]                    = ImVec4( 0.51f, 0.77f, 0.99f, 0.63f );
    colors[ImGuiCol_ButtonHovered]             = ImVec4( 0.51f, 0.77f, 0.99f, 0.68f );
    colors[ImGuiCol_ButtonActive]              = ImVec4( 0.24f, 0.73f, 0.99f, 1.00f );
    colors[ImGuiCol_Header]                    = ImVec4( 0.51f, 0.77f, 0.99f, 0.56f );
    colors[ImGuiCol_HeaderHovered]             = ImVec4( 0.51f, 0.77f, 0.99f, 0.89f );
    colors[ImGuiCol_HeaderActive]              = ImVec4( 0.51f, 0.77f, 0.99f, 1.00f );
    colors[ImGuiCol_Separator]                 = ImVec4( 1.00f, 1.00f, 1.00f, 0.71f );
    colors[ImGuiCol_SeparatorHovered]          = ImVec4( 0.32f, 0.63f, 0.87f, 0.88f );
    colors[ImGuiCol_SeparatorActive]           = ImVec4( 0.32f, 0.63f, 0.87f, 1.00f );
    colors[ImGuiCol_ResizeGrip]                = ImVec4( 0.51f, 0.77f, 0.99f, 0.45f );
    colors[ImGuiCol_ResizeGripHovered]         = ImVec4( 0.51f, 0.77f, 0.99f, 0.82f );
    colors[ImGuiCol_ResizeGripActive]          = ImVec4( 0.51f, 0.77f, 0.99f, 0.97f );
    colors[ImGuiCol_InputTextCursor]           = ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );
    colors[ImGuiCol_TabHovered]                = ImVec4( 0.51f, 0.77f, 0.99f, 0.89f );
    colors[ImGuiCol_Tab]                       = ImVec4( 0.42f, 0.59f, 0.76f, 0.93f );
    colors[ImGuiCol_TabSelected]               = ImVec4( 0.45f, 0.64f, 0.82f, 1.00f );
    colors[ImGuiCol_TabSelectedOverline]       = ImVec4( 0.51f, 0.77f, 0.99f, 1.00f );
    colors[ImGuiCol_TabDimmed]                 = ImVec4( 0.26f, 0.32f, 0.38f, 0.99f );
    colors[ImGuiCol_TabDimmedSelected]         = ImVec4( 0.37f, 0.51f, 0.65f, 1.00f );
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4( 0.71f, 0.71f, 0.71f, 0.00f );
    colors[ImGuiCol_PlotLines]                 = ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );
    colors[ImGuiCol_PlotLinesHovered]          = ImVec4( 1.00f, 0.66f, 0.59f, 1.00f );
    colors[ImGuiCol_PlotHistogram]             = ImVec4( 0.95f, 0.84f, 0.00f, 1.00f );
    colors[ImGuiCol_PlotHistogramHovered]      = ImVec4( 1.00f, 0.77f, 0.00f, 1.00f );
    colors[ImGuiCol_TableHeaderBg]             = ImVec4( 0.44f, 0.44f, 0.45f, 1.00f );
    colors[ImGuiCol_TableBorderStrong]         = ImVec4( 0.56f, 0.56f, 0.59f, 1.00f );
    colors[ImGuiCol_TableBorderLight]          = ImVec4( 0.48f, 0.48f, 0.50f, 1.00f );
    colors[ImGuiCol_TableRowBg]                = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
    colors[ImGuiCol_TableRowBgAlt]             = ImVec4( 1.00f, 1.00f, 1.00f, 0.24f );
    colors[ImGuiCol_TextLink]                  = ImVec4( 0.51f, 0.77f, 0.99f, 1.00f );
    colors[ImGuiCol_TextSelectedBg]            = ImVec4( 0.51f, 0.77f, 0.99f, 0.59f );
    colors[ImGuiCol_TreeLines]                 = ImVec4( 0.66f, 0.66f, 0.71f, 0.71f );
    colors[ImGuiCol_DragDropTarget]            = ImVec4( 1.00f, 1.00f, 0.00f, 0.95f );
    colors[ImGuiCol_DragDropTargetBg]          = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
    colors[ImGuiCol_UnsavedMarker]             = ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );
    colors[ImGuiCol_NavCursor]                 = ImVec4( 0.51f, 0.77f, 0.99f, 1.00f );
    colors[ImGuiCol_NavWindowingHighlight]     = ImVec4( 1.00f, 1.00f, 1.00f, 0.84f );
    colors[ImGuiCol_NavWindowingDimBg]         = ImVec4( 0.89f, 0.89f, 0.89f, 0.45f );
    colors[ImGuiCol_ModalWindowDimBg]          = ImVec4( 0.89f, 0.89f, 0.89f, 0.59f );
}

static void StyleColorsNcoreDark()
{
    auto& colors                               = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                      = ImVec4( 0.92f, 0.96f, 0.98f, 1.00f ); // near-white, slight cyan cast
    colors[ImGuiCol_TextDisabled]              = ImVec4( 0.45f, 0.55f, 0.62f, 1.00f );
    colors[ImGuiCol_WindowBg]                  = ImVec4( 0.04f, 0.09f, 0.14f, 0.94f ); // very dark navy
    colors[ImGuiCol_ChildBg]                   = ImVec4( 0.03f, 0.07f, 0.11f, 0.60f );
    colors[ImGuiCol_PopupBg]                   = ImVec4( 0.05f, 0.11f, 0.17f, 0.97f );
    colors[ImGuiCol_Border]                    = ImVec4( 0.22f, 0.45f, 0.58f, 0.55f ); // muted teal border
    colors[ImGuiCol_BorderShadow]              = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
    colors[ImGuiCol_FrameBg]                   = ImVec4( 0.08f, 0.18f, 0.26f, 0.80f );
    colors[ImGuiCol_FrameBgHovered]            = ImVec4( 0.12f, 0.32f, 0.45f, 0.75f );
    colors[ImGuiCol_FrameBgActive]             = ImVec4( 0.15f, 0.42f, 0.58f, 0.85f );
    colors[ImGuiCol_TitleBg]                   = ImVec4( 0.03f, 0.08f, 0.13f, 1.00f );
    colors[ImGuiCol_TitleBgActive]             = ImVec4( 0.06f, 0.16f, 0.25f, 1.00f );
    colors[ImGuiCol_TitleBgCollapsed]          = ImVec4( 0.02f, 0.05f, 0.08f, 0.75f );
    colors[ImGuiCol_MenuBarBg]                 = ImVec4( 0.04f, 0.10f, 0.15f, 1.00f );
    colors[ImGuiCol_ScrollbarBg]               = ImVec4( 0.02f, 0.04f, 0.07f, 0.45f );
    colors[ImGuiCol_ScrollbarGrab]             = ImVec4( 0.18f, 0.38f, 0.50f, 0.80f );
    colors[ImGuiCol_ScrollbarGrabHovered]      = ImVec4( 0.28f, 0.55f, 0.70f, 0.90f );
    colors[ImGuiCol_ScrollbarGrabActive]       = ImVec4( 0.35f, 0.70f, 0.88f, 1.00f );
    colors[ImGuiCol_CheckMark]                 = ImVec4( 0.35f, 0.85f, 0.95f, 1.00f );
    colors[ImGuiCol_SliderGrab]                = ImVec4( 0.30f, 0.75f, 0.90f, 1.00f );
    colors[ImGuiCol_SliderGrabActive]          = ImVec4( 0.45f, 0.90f, 1.00f, 1.00f );
    colors[ImGuiCol_Button]                    = ImVec4( 0.12f, 0.35f, 0.48f, 0.70f );
    colors[ImGuiCol_ButtonHovered]             = ImVec4( 0.18f, 0.50f, 0.68f, 0.85f );
    colors[ImGuiCol_ButtonActive]              = ImVec4( 0.25f, 0.70f, 0.90f, 1.00f );
    colors[ImGuiCol_Header]                    = ImVec4( 0.14f, 0.38f, 0.52f, 0.55f );
    colors[ImGuiCol_HeaderHovered]             = ImVec4( 0.20f, 0.52f, 0.70f, 0.80f );
    colors[ImGuiCol_HeaderActive]              = ImVec4( 0.25f, 0.65f, 0.85f, 0.95f );
    colors[ImGuiCol_Separator]                 = ImVec4( 0.25f, 0.50f, 0.62f, 0.50f );
    colors[ImGuiCol_SeparatorHovered]          = ImVec4( 0.35f, 0.75f, 0.90f, 0.78f );
    colors[ImGuiCol_SeparatorActive]           = ImVec4( 0.40f, 0.85f, 1.00f, 1.00f );
    colors[ImGuiCol_ResizeGrip]                = ImVec4( 0.25f, 0.55f, 0.70f, 0.40f );
    colors[ImGuiCol_ResizeGripHovered]         = ImVec4( 0.35f, 0.75f, 0.90f, 0.70f );
    colors[ImGuiCol_ResizeGripActive]          = ImVec4( 0.45f, 0.90f, 1.00f, 0.95f );
    colors[ImGuiCol_Tab]                       = ImVec4( 0.10f, 0.25f, 0.35f, 0.90f );
    colors[ImGuiCol_TabHovered]                = ImVec4( 0.20f, 0.50f, 0.68f, 0.90f );
    colors[ImGuiCol_TabSelected]               = ImVec4( 0.15f, 0.40f, 0.55f, 1.00f );
    colors[ImGuiCol_TabSelectedOverline]       = ImVec4( 0.40f, 0.85f, 1.00f, 1.00f );
    colors[ImGuiCol_TabDimmed]                 = ImVec4( 0.08f, 0.16f, 0.22f, 0.95f );
    colors[ImGuiCol_TabDimmedSelected]         = ImVec4( 0.12f, 0.30f, 0.42f, 1.00f );
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4( 0.30f, 0.60f, 0.75f, 0.60f );
    colors[ImGuiCol_PlotLines]                 = ImVec4( 0.40f, 0.85f, 0.95f, 1.00f );
    colors[ImGuiCol_PlotLinesHovered]          = ImVec4( 0.70f, 0.95f, 1.00f, 1.00f );
    colors[ImGuiCol_PlotHistogram]             = ImVec4( 0.25f, 0.70f, 0.85f, 1.00f );
    colors[ImGuiCol_PlotHistogramHovered]      = ImVec4( 0.45f, 0.90f, 1.00f, 1.00f );
    colors[ImGuiCol_TableHeaderBg]             = ImVec4( 0.08f, 0.20f, 0.30f, 1.00f );
    colors[ImGuiCol_TableBorderStrong]         = ImVec4( 0.20f, 0.42f, 0.55f, 1.00f );
    colors[ImGuiCol_TableBorderLight]          = ImVec4( 0.15f, 0.30f, 0.40f, 1.00f );
    colors[ImGuiCol_TableRowBg]                = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
    colors[ImGuiCol_TableRowBgAlt]             = ImVec4( 0.10f, 0.22f, 0.32f, 0.25f );
    colors[ImGuiCol_TextLink]                  = ImVec4( 0.40f, 0.85f, 1.00f, 1.00f );
    colors[ImGuiCol_TextSelectedBg]            = ImVec4( 0.20f, 0.55f, 0.75f, 0.45f );
    colors[ImGuiCol_DragDropTarget]            = ImVec4( 0.40f, 0.90f, 1.00f, 0.90f );
    colors[ImGuiCol_NavCursor]                 = ImVec4( 0.40f, 0.85f, 1.00f, 1.00f );
    colors[ImGuiCol_NavWindowingHighlight]     = ImVec4( 0.50f, 0.90f, 1.00f, 0.70f );
    colors[ImGuiCol_NavWindowingDimBg]         = ImVec4( 0.02f, 0.05f, 0.08f, 0.55f );
    colors[ImGuiCol_ModalWindowDimBg]          = ImVec4( 0.01f, 0.03f, 0.06f, 0.65f );
    colors[ImGuiCol_InputTextCursor]           = ImVec4( 0.50f, 0.95f, 1.00f, 1.00f );
    colors[ImGuiCol_TreeLines]                 = ImVec4( 0.25f, 0.50f, 0.62f, 0.50f );
    colors[ImGuiCol_UnsavedMarker]             = ImVec4( 0.40f, 0.90f, 1.00f, 1.00f );
}

static void StyleSizesNcore()
{
    auto& style          = ImGui::GetStyle();
    style.FontSizeBase   = 18.0f;
    style.WindowRounding = 10.0f;
    style.FrameRounding  = 5.0f;
}

static void StyleSizesNcoreDark()
{
    auto& style             = ImGui::GetStyle();
    style.FontSizeBase      = 18.0f;
    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 4.0f;
    style.FrameRounding     = 3.0f;
    style.PopupRounding     = 5.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 4.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.WindowPadding     = ImVec2( 10.0f, 8.0f );
    style.FramePadding      = ImVec2( 8.0f, 4.0f );
    style.ItemSpacing       = ImVec2( 8.0f, 5.0f );
    style.ItemInnerSpacing  = ImVec2( 5.0f, 4.0f );
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 10.0f;
}

static CursorType cursor_type_to_imgui_cursor( ImGuiMouseCursor cursor )
{
    switch (cursor) {
        case ImGuiMouseCursor_Arrow:
            return CursorType::DEFAULT;
        case ImGuiMouseCursor_TextInput:
            return CursorType::TEXT;
        case ImGuiMouseCursor_ResizeNS:
            return CursorType::RESIZE_NS;
        case ImGuiMouseCursor_ResizeEW:
            return CursorType::RESIZE_EW;
        case ImGuiMouseCursor_ResizeNESW:
            return CursorType::RESIZE_NESW;
        case ImGuiMouseCursor_ResizeNWSE:
            return CursorType::RESIZE_NWSE;
        case ImGuiMouseCursor_Hand:
            return CursorType::POINTER;
        case ImGuiMouseCursor_Wait:
            return CursorType::WAIT;
        default:
            return CursorType::DEFAULT;
    }
}

static ImGuiKey key_to_imgui_key( Key key )
{
    switch (key) {
        case Key::W:
            return ImGuiKey_W;
        case Key::A:
            return ImGuiKey_A;
        case Key::S:
            return ImGuiKey_S;
        case Key::D:
            return ImGuiKey_D;

        case Key::B:
            return ImGuiKey_B;
        case Key::C:
            return ImGuiKey_C;
        case Key::E:
            return ImGuiKey_E;
        case Key::F:
            return ImGuiKey_F;
        case Key::G:
            return ImGuiKey_G;
        case Key::H:
            return ImGuiKey_H;
        case Key::I:
            return ImGuiKey_I;
        case Key::J:
            return ImGuiKey_J;
        case Key::K:
            return ImGuiKey_K;
        case Key::L:
            return ImGuiKey_L;
        case Key::M:
            return ImGuiKey_M;
        case Key::N:
            return ImGuiKey_N;
        case Key::O:
            return ImGuiKey_O;
        case Key::P:
            return ImGuiKey_P;
        case Key::Q:
            return ImGuiKey_Q;
        case Key::R:
            return ImGuiKey_R;
        case Key::T:
            return ImGuiKey_T;
        case Key::U:
            return ImGuiKey_U;
        case Key::V:
            return ImGuiKey_V;
        case Key::X:
            return ImGuiKey_X;
        case Key::Y:
            return ImGuiKey_Y;
        case Key::Z:
            return ImGuiKey_Z;

        case Key::_0:
            return ImGuiKey_0;
        case Key::_1:
            return ImGuiKey_1;
        case Key::_2:
            return ImGuiKey_2;
        case Key::_3:
            return ImGuiKey_3;
        case Key::_4:
            return ImGuiKey_4;
        case Key::_5:
            return ImGuiKey_5;
        case Key::_6:
            return ImGuiKey_6;
        case Key::_7:
            return ImGuiKey_7;
        case Key::_8:
            return ImGuiKey_8;
        case Key::_9:
            return ImGuiKey_9;

        case Key::F1:
            return ImGuiKey_F1;
        case Key::F2:
            return ImGuiKey_F2;
        case Key::F3:
            return ImGuiKey_F3;
        case Key::F4:
            return ImGuiKey_F4;
        case Key::F5:
            return ImGuiKey_F5;
        case Key::F6:
            return ImGuiKey_F6;
        case Key::F7:
            return ImGuiKey_F7;
        case Key::F8:
            return ImGuiKey_F8;
        case Key::F9:
            return ImGuiKey_F9;
        case Key::F10:
            return ImGuiKey_F10;
        case Key::F11:
            return ImGuiKey_F11;
        case Key::F12:
            return ImGuiKey_F12;

        case Key::UP:
            return ImGuiKey_UpArrow;
        case Key::DOWN:
            return ImGuiKey_DownArrow;
        case Key::LEFT:
            return ImGuiKey_LeftArrow;
        case Key::RIGHT:
            return ImGuiKey_RightArrow;
        case Key::HOME:
            return ImGuiKey_Home;
        case Key::END:
            return ImGuiKey_End;
        case Key::PAGEUP:
            return ImGuiKey_PageUp;
        case Key::PAGEDOWN:
            return ImGuiKey_PageDown;
        case Key::INSERT:
            return ImGuiKey_Insert;
        case Key::DEL:
            return ImGuiKey_Delete;

        case Key::SPACE:
            return ImGuiKey_Space;
        case Key::ENTER:
            return ImGuiKey_Enter;
        case Key::ESC:
            return ImGuiKey_Escape;
        case Key::TAB:
            return ImGuiKey_Tab;
        case Key::BKSP:
            return ImGuiKey_Backspace;
        case Key::SHIFT:
            return ImGuiKey_LeftShift;
        case Key::CTRL:
            return ImGuiKey_LeftCtrl;
        case Key::ALT:
            return ImGuiKey_LeftAlt;

        case Key::PERIOD:
            return ImGuiKey_Period;
        case Key::COMMA:
            return ImGuiKey_Comma;
        case Key::MINUS:
            return ImGuiKey_Minus;
        case Key::EQUALS:
            return ImGuiKey_Equal;
        case Key::SEMICOLON:
            return ImGuiKey_Semicolon;
        case Key::QUOTE:
            return ImGuiKey_Apostrophe;
        case Key::BACKSLASH:
            return ImGuiKey_Backslash;
        case Key::SLASH:
            return ImGuiKey_Slash;
        case Key::LBRACKET:
            return ImGuiKey_LeftBracket;
        case Key::RBRACKET:
            return ImGuiKey_RightBracket;
        case Key::BACKQUOTE:
            return ImGuiKey_GraveAccent;

        case Key::CAPSLOCK:
            return ImGuiKey_CapsLock;
        case Key::APPS:
            return ImGuiKey_None;

        case Key::UNKNOWN:
        case Key::COUNT:
            return ImGuiKey_None;
    }
    return ImGuiKey_None;
}

} // namespace nc
