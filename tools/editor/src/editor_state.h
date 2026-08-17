#include <ncore/core/types.h>

namespace nc::editor {

struct EngineEditorState {
    Scene* CurrentScene        = nullptr;
    ImGuiID DockspaceId        = 0;
    bool ShowStatsWindow       = false;
    bool ShowInputsWindow      = false;
    bool ShowLogsWindow        = false;
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
        EngineEditorState, NC_F( EngineEditorState, CurrentScene ), NC_F( EngineEditorState, DockspaceId ),
        NC_F( EngineEditorState, ShowStatsWindow ), NC_F( EngineEditorState, ShowInputsWindow ),
        NC_F( EngineEditorState, ShowLogsWindow ), NC_F( EngineEditorState, SelectedNode )
    )
};

} // namespace nc::editor
