#pragma once

#include <imgui.h>

#include <ncore/core/types.h>
#include <ncore/services/video/window/window_types.h>

namespace nc::editor {

struct GuiStateComponent {
    ImGuiContext* ImGuiCtx = nullptr;
    HashMap<ImGuiMouseCursor, nc::CursorType> CursorMap;
    RID Material;
    NSTRUCTV(
        GuiStateComponent, NC_F( GuiStateComponent, ImGuiCtx ), NC_F( GuiStateComponent, CursorMap ),
        NC_F( GuiStateComponent, Material )
    )
};

struct EditorState {
    Scene* CurrentScene        = nullptr;
    RID EditorCamSource        = 0;
    RID ViewportRT             = 0; // render texture.
    RID ViewportDT             = 0; // depth texture.
    Vec2f ViewportSize         = Vec2f();
    RID GameViewRT             = 0; // offscreen RT for game cameras.
    RID GameViewDT             = 0; // offscreen depth for game cameras.
    Vec2f GameViewSize         = Vec2f();
    bool ViewportFocused       = false;
    bool ViewportHovered       = false;
    ImGuiID DockspaceId        = 0;
    bool ShowStatsWindow       = false;
    bool ShowInputsWindow      = false;
    bool ShowLogsWindow        = true;
    Node* SelectedNode         = nullptr;
    char NodeRenameBuf[256]    = {};
    Node* NodeToRename         = nullptr;
    bool ShowRenamePopup       = false;
    char AddCompFilter[64]     = {};
    ImGuiTextBuffer LogsBuffer = {};
    ImGuiTextFilter LogsFilter = {};
    ImVector<int> LogsOffset;
    bool LogsAutoScroll = true;
    log::ListenerToken LogsListenerToken; // for automatic de-registration

    Vec3 GridPos      = Vec3( 0, -15, 0 );
    Vec3 GridRotation = Vec3( 0, 90, 0 );
    Vec3 GridScale    = Vec3( 1, 1, 1 );

    bool GlobalXformGizmo   = false;
    int XformGizmoOperation = 14463; // default = universal op
    bool DrawWireframe      = 0;

    NSTRUCTV(
        EditorState, NC_F( EditorState, CurrentScene ), NC_F( EditorState, EditorCamSource ),
        NC_F( EditorState, ViewportRT ), NC_F( EditorState, ViewportDT ), NC_F( EditorState, ViewportSize ),
        NC_F( EditorState, GameViewRT ), NC_F( EditorState, GameViewDT ), NC_F( EditorState, GameViewSize ),
        NC_F( EditorState, ViewportFocused ), NC_F( EditorState, ViewportHovered ), NC_F( EditorState, DockspaceId ),
        NC_F( EditorState, ShowStatsWindow ), NC_F( EditorState, ShowInputsWindow ),
        NC_F( EditorState, ShowLogsWindow ), NC_F( EditorState, SelectedNode )
    )
};

} // namespace nc::editor
