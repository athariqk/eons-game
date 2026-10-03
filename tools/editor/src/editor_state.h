#pragma once

#include <imgui.h>

#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/video/window/window_types.h>

namespace nc::editor {

NC_COMPONENT( EditorCameraTag ){};

NC_COMPONENT( GuiStateComponent )
{
    ImGuiContext* ImGuiCtx = nullptr;
    HashMap<ImGuiMouseCursor, nc::CursorType> CursorMap;
    RID CanvasItem;

    NC_PROPS_BEGIN()
    ADD_PROPERTY( ImGuiCtx )
    ADD_PROPERTY( CursorMap )
    ADD_PROPERTY( CanvasItem )
    NC_PROPS_END()
};

NC_COMPONENT( EditorState )
{
    Scene* CurrentScene        = nullptr;
    RID EditorCamSource        = 0;
    RID ViewportRT             = 0; // render texture.
    RID ViewportDT             = 0; // depth texture.
    Vec2f ViewportSize         = Vec2f();
    bool ShowGameView          = true;
    RID GameViewRT             = 0; // offscreen RT for game cameras.
    RID GameViewDT             = 0; // offscreen depth for game cameras.
    Vec2f GameViewSize         = Vec2f();
    bool RenderGameView        = true;
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

    NC_PROPS_BEGIN()
    ADD_PROPERTY( CurrentScene )
    ADD_PROPERTY( EditorCamSource )
    ADD_PROPERTY( ViewportRT )
    ADD_PROPERTY( ViewportDT )
    ADD_PROPERTY( ViewportSize )
    ADD_PROPERTY( ShowGameView )
    ADD_PROPERTY( GameViewRT )
    ADD_PROPERTY( GameViewDT )
    ADD_PROPERTY( GameViewSize )
    ADD_PROPERTY( RenderGameView )
    ADD_PROPERTY( ViewportFocused )
    ADD_PROPERTY( ViewportHovered )
    ADD_PROPERTY( DockspaceId )
    ADD_PROPERTY( ShowStatsWindow )
    ADD_PROPERTY( ShowInputsWindow )
    ADD_PROPERTY( ShowLogsWindow )
    ADD_PROPERTY( SelectedNode )
    NC_PROPS_END()
};

} // namespace nc::editor
