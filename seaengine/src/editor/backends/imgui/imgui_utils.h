#pragma once

#include <imgui.h>

#include <ncore/services/io/input_event.h>
#include <ncore/services/video/window/window_types.h>

namespace sea::editor {

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

static nc::CursorType cursor_type_to_imgui_cursor( ImGuiMouseCursor cursor )
{
    switch (cursor) {
        case ImGuiMouseCursor_Arrow:
            return nc::CursorType::DEFAULT;
        case ImGuiMouseCursor_TextInput:
            return nc::CursorType::TEXT;
        case ImGuiMouseCursor_ResizeNS:
            return nc::CursorType::RESIZE_NS;
        case ImGuiMouseCursor_ResizeEW:
            return nc::CursorType::RESIZE_EW;
        case ImGuiMouseCursor_ResizeNESW:
            return nc::CursorType::RESIZE_NESW;
        case ImGuiMouseCursor_ResizeNWSE:
            return nc::CursorType::RESIZE_NWSE;
        case ImGuiMouseCursor_Hand:
            return nc::CursorType::POINTER;
        case ImGuiMouseCursor_Wait:
            return nc::CursorType::WAIT;
        default:
            return nc::CursorType::DEFAULT;
    }
}

static ImGuiKey key_to_imgui_key( nc::Key key )
{
    switch (key) {
        case nc::Key::W:
            return ImGuiKey_W;
        case nc::Key::A:
            return ImGuiKey_A;
        case nc::Key::S:
            return ImGuiKey_S;
        case nc::Key::D:
            return ImGuiKey_D;

        case nc::Key::B:
            return ImGuiKey_B;
        case nc::Key::C:
            return ImGuiKey_C;
        case nc::Key::E:
            return ImGuiKey_E;
        case nc::Key::F:
            return ImGuiKey_F;
        case nc::Key::G:
            return ImGuiKey_G;
        case nc::Key::H:
            return ImGuiKey_H;
        case nc::Key::I:
            return ImGuiKey_I;
        case nc::Key::J:
            return ImGuiKey_J;
        case nc::Key::K:
            return ImGuiKey_K;
        case nc::Key::L:
            return ImGuiKey_L;
        case nc::Key::M:
            return ImGuiKey_M;
        case nc::Key::N:
            return ImGuiKey_N;
        case nc::Key::O:
            return ImGuiKey_O;
        case nc::Key::P:
            return ImGuiKey_P;
        case nc::Key::Q:
            return ImGuiKey_Q;
        case nc::Key::R:
            return ImGuiKey_R;
        case nc::Key::T:
            return ImGuiKey_T;
        case nc::Key::U:
            return ImGuiKey_U;
        case nc::Key::V:
            return ImGuiKey_V;
        case nc::Key::X:
            return ImGuiKey_X;
        case nc::Key::Y:
            return ImGuiKey_Y;
        case nc::Key::Z:
            return ImGuiKey_Z;

        case nc::Key::_0:
            return ImGuiKey_0;
        case nc::Key::_1:
            return ImGuiKey_1;
        case nc::Key::_2:
            return ImGuiKey_2;
        case nc::Key::_3:
            return ImGuiKey_3;
        case nc::Key::_4:
            return ImGuiKey_4;
        case nc::Key::_5:
            return ImGuiKey_5;
        case nc::Key::_6:
            return ImGuiKey_6;
        case nc::Key::_7:
            return ImGuiKey_7;
        case nc::Key::_8:
            return ImGuiKey_8;
        case nc::Key::_9:
            return ImGuiKey_9;

        case nc::Key::F1:
            return ImGuiKey_F1;
        case nc::Key::F2:
            return ImGuiKey_F2;
        case nc::Key::F3:
            return ImGuiKey_F3;
        case nc::Key::F4:
            return ImGuiKey_F4;
        case nc::Key::F5:
            return ImGuiKey_F5;
        case nc::Key::F6:
            return ImGuiKey_F6;
        case nc::Key::F7:
            return ImGuiKey_F7;
        case nc::Key::F8:
            return ImGuiKey_F8;
        case nc::Key::F9:
            return ImGuiKey_F9;
        case nc::Key::F10:
            return ImGuiKey_F10;
        case nc::Key::F11:
            return ImGuiKey_F11;
        case nc::Key::F12:
            return ImGuiKey_F12;

        case nc::Key::UP:
            return ImGuiKey_UpArrow;
        case nc::Key::DOWN:
            return ImGuiKey_DownArrow;
        case nc::Key::LEFT:
            return ImGuiKey_LeftArrow;
        case nc::Key::RIGHT:
            return ImGuiKey_RightArrow;
        case nc::Key::HOME:
            return ImGuiKey_Home;
        case nc::Key::END:
            return ImGuiKey_End;
        case nc::Key::PAGEUP:
            return ImGuiKey_PageUp;
        case nc::Key::PAGEDOWN:
            return ImGuiKey_PageDown;
        case nc::Key::INSERT:
            return ImGuiKey_Insert;
        case nc::Key::DEL:
            return ImGuiKey_Delete;

        case nc::Key::SPACE:
            return ImGuiKey_Space;
        case nc::Key::ENTER:
            return ImGuiKey_Enter;
        case nc::Key::ESC:
            return ImGuiKey_Escape;
        case nc::Key::TAB:
            return ImGuiKey_Tab;
        case nc::Key::BKSP:
            return ImGuiKey_Backspace;
        case nc::Key::SHIFT:
            return ImGuiKey_LeftShift;
        case nc::Key::CTRL:
            return ImGuiKey_LeftCtrl;
        case nc::Key::ALT:
            return ImGuiKey_LeftAlt;

        case nc::Key::PERIOD:
            return ImGuiKey_Period;
        case nc::Key::COMMA:
            return ImGuiKey_Comma;
        case nc::Key::MINUS:
            return ImGuiKey_Minus;
        case nc::Key::EQUALS:
            return ImGuiKey_Equal;
        case nc::Key::SEMICOLON:
            return ImGuiKey_Semicolon;
        case nc::Key::QUOTE:
            return ImGuiKey_Apostrophe;
        case nc::Key::BACKSLASH:
            return ImGuiKey_Backslash;
        case nc::Key::SLASH:
            return ImGuiKey_Slash;
        case nc::Key::LBRACKET:
            return ImGuiKey_LeftBracket;
        case nc::Key::RBRACKET:
            return ImGuiKey_RightBracket;
        case nc::Key::BACKQUOTE:
            return ImGuiKey_GraveAccent;

        case nc::Key::CAPSLOCK:
            return ImGuiKey_CapsLock;
        case nc::Key::APPS:
            return ImGuiKey_None;

        case nc::Key::UNKNOWN:
        case nc::Key::COUNT:
            return ImGuiKey_None;
    }
    return ImGuiKey_None;
}

} // namespace sea::editor
