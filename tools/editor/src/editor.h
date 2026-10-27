#pragma once

// clang-format off
#include <imgui_internal.h>
#include <ImGuizmo.h> // must come after imgui includes
// clang-format on

#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>

#include <imgui.h>

#include <ncore/core/collection.h>
#include <ncore/core/memory.h>
#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_entity.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/utils/log.h>

#include "editor_components.h"

namespace nc {
class EcsWorld;
class InputService;
class Node;
class RenderService;
class ResourceLoader;
class Scene;
struct TimeComponent;

namespace editor {
class AssetIndex;
class AssetImporter;
} // namespace editor
} // namespace nc

namespace nc::editor {

//------------------------------------------------------------------------------
// Selection
//------------------------------------------------------------------------------

/**
 * @brief The editor selection: ECS-backed, with the current id cached.
 *
 * SelectedTag stays in the world so engine-side systems can read it; this class
 * is the editor-facing API and keeps the id for cheap per-frame comparisons.
 */
class Selection {
public:
    /** @brief Re-read the tag (picks up selection changes made outside the editor). */
    void refresh( EcsWorld& world );
    void select( EcsWorld& world, EcsEntity entity );
    void clear( EcsWorld& world );

    EcsEntity get_selected_eid() const
    {
        return selected_eid;
    }

    Node* node( Scene& scene ) const;

private:
    EcsEntity selected_eid = INVALID_ENTITY_ID;
};

//------------------------------------------------------------------------------
// Log store
//------------------------------------------------------------------------------

/**
 * @brief The editor console state holding line buffer, per-line metadata and filters.
 *
 * Lines are staged by whichever thread logs them (see Editor::stage_log) and
 * drained on the render thread, so the buffer stays bounded while the panel is
 * closed.
 */
struct LogStore {
    ImGuiTextBuffer buffer = {};
    ImGuiTextFilter filter = {};
    ImVector<int> offset; // line starts plus a trailing sentinel (count = Size - 1)
    ImVector<uint8_t> level;
    ImVector<int32_t> channel;
    DynamicArray<String> channels;

    bool is_level_enabled[6] = { true, true, true, true, true, true };
    int channel_filter       = -1;
    bool is_autoscroll       = true;

    void clear();

    static ImVec4 level_color( int level );
};

//------------------------------------------------------------------------------
// Frame context
//------------------------------------------------------------------------------

struct EditorCommand;
class InspectorHandler;
struct PropertyWidget;

/**
 * @brief Everything a panel needs to draw one frame.
 */
struct EditorContext {
    Scene* scene               = nullptr;
    EcsWorld* ecs_world        = nullptr;
    ResourceLoader* loader     = nullptr;
    RenderService* renderer    = nullptr;
    InputService* inputs       = nullptr;
    TimeComponent* time        = nullptr;
    AssetIndex* assets         = nullptr;
    AssetImporter* importer    = nullptr;
    Selection* selection_state = nullptr;
    LogStore* log_store        = nullptr;

    EcsEntity selected_eid = INVALID_ENTITY_ID;

    // Toolbar/gizmo preferences, owned by the editor and read by the overlays.
    bool global_xform_gizmo   = false;
    int xform_gizmo_operation = 14463; // ImGuizmo::OPERATION::UNIVERSAL
    bool draw_wireframe       = false;

    // Published by the viewport panels while drawing.
    bool any_viewport_hovered = false;
    bool any_viewport_focused = false;

    // Contributions registered by plugins (drawn by the toolbar / menu bar).
    const DynamicArray<EditorCommand>* commands = nullptr;

    const DynamicArray<Ptr<InspectorHandler>>* inspector_handlers = nullptr;
    const DynamicArray<PropertyWidget>* property_widgets          = nullptr;
};

//------------------------------------------------------------------------------
// Inspector extension points
//------------------------------------------------------------------------------

/** @brief Layout constants shared by the inspector sheet and its handlers. */
inline constexpr float kInspectorLabelWidth = 140.0f;
inline constexpr float kInspectorIndentPx   = 12.0f;

/**
 * @brief Per-component inspector extension (Godot's EditorInspectorPlugin).
 *
 * Handlers run after the default property sheet for every component they
 * accept, so a feature can add a preview/section without owning the sheet.
 */
class InspectorHandler {
public:
    virtual ~InspectorHandler() = default;

    virtual bool can_handle( const rtti::TypeInfo* type, void* component ) const = 0;

    virtual void draw( EditorContext& ctx, Node& node, const rtti::TypeInfo* type, void* component ) = 0;

    /** @brief Skip the generic property sheet for components this handler owns. */
    virtual bool replaces_default() const
    {
        return false;
    }
};

/**
 * @brief Custom value widget for one reflected type (RID, enums, etc).
 *
 * Return true when the widget owned the row; false falls back to the default.
 */
using PropertyWidgetFn = std::function<bool(
    EditorContext& ctx, const rtti::RecordInfo* owner, StringView field, const rtti::TypeInfo* type, void* value,
    bool& out_changed
)>;

struct PropertyWidget {
    const rtti::TypeInfo* Type = nullptr;
    PropertyWidgetFn Draw;
    int owner = -1;
};

//------------------------------------------------------------------------------
// Panels
//------------------------------------------------------------------------------

/** @brief Named dock destinations. */
enum class DockSlot : uint8_t {
    Top,
    LeftUpper,
    LeftLower,
    RightUpper,
    RightLower,
    Bottom,
    Center,
    Floating,
};

/**
 * @brief One editor window.
 *
 * Panels are ordinary objects: the editor owns them, draws them in (slot, order)
 * order and builds the dock layout from them.
 */
class EditorPanel {
public:
    EditorPanel(
        const String& p_title, DockSlot p_slot, int p_order = 0, bool p_visible = true, bool p_closable = true
    ) : title( p_title ), slot( p_slot ), order( p_order ), is_visible( p_visible ), is_closable( p_closable )
    {}

    virtual ~EditorPanel() = default;

    EditorPanel( const EditorPanel& )            = delete;
    EditorPanel& operator=( const EditorPanel& ) = delete;

    const String& get_title() const
    {
        return title;
    }

    DockSlot get_slot() const
    {
        return slot;
    }
    /**
     * @brief Set by Editor::register_panel(); the registration call wins over the ctor default.
     */
    void set_slot( DockSlot p_slot )
    {
        slot = p_slot;
    }

    int get_order() const
    {
        return order;
    }
    void set_order( int p_order )
    {
        order = p_order;
    }

    bool get_visible() const
    {
        return is_visible;
    }

    bool& get_visible()
    {
        return is_visible;
    }

    bool get_closable() const
    {
        return is_closable;
    }

    /**
     * @brief Get the index of the plugin that registered this panel (-1 = editor core).
     */
    int get_owner() const
    {
        return owner;
    }
    void set_owner( int p_owner )
    {
        owner = p_owner;
    }

    /**
     * @brief Draw the panel. Called every frame, even when hidden.
     */
    virtual void draw( EditorContext& ctx ) = 0;

    /**
     * @brief Release GPU resources owned by the panel (render targets, thumbnails).
     */
    virtual void release_gpu( RenderService& renderer )
    {
        ( void ) renderer;
    }

protected:
    String title;
    DockSlot slot;
    int order;
    bool is_visible;
    bool is_closable;
    int owner = -1;
};

//------------------------------------------------------------------------------
// Plugin interface
//------------------------------------------------------------------------------

/**
 * @brief A menu / toolbar entry contributed by a plugin.
 */
struct EditorCommand {
    String menu; // empty = toolbar button
    String label;
    std::function<void()> invoke;
    int owner = -1;
};

class Editor;

/**
 * @brief The only surface plugins use to extend the editor.
 *
 * Verbs rather than data: a plugin says what the editor should do with an
 * object, and the editor keeps track so it can be unregistered as a unit.
 */
class EditorInterface {
public:
    explicit EditorInterface( Editor& p_editor ) : editor( p_editor ) {}

    /** @brief Add a panel to a dock slot; order controls the tab order. */
    EditorPanel* add_panel( DockSlot slot, int order, Ptr<EditorPanel> panel );

    /** @brief Add an entry to a menu ("File", "View", ...) or a toolbar button (""). */
    void add_command( StringView menu, StringView label, std::function<void()> on_invoke );

    /** @brief Add a per-component inspector extension. */
    void add_inspector_handler( Ptr<InspectorHandler> handler );

    /** @brief Override the value widget for a reflected type. */
    void add_property_widget( const rtti::TypeInfo* type, PropertyWidgetFn widget );

    EditorContext& get_ctx() const;
    Scene& get_scene() const;
    Editor& get_editor() const;

private:
    Editor& editor;
};

/**
 * @brief A unit of editor functionality.
 *
 * Plugins register their panels/commands/overlays in enter(); the editor owns
 * the registry and drops everything the plugin added when it is removed.
 */
class EditorPlugin {
public:
    virtual ~EditorPlugin() = default;

    virtual StringView name() const = 0;

    virtual void enter( EditorInterface& ed ) = 0;

    virtual void exit( EditorInterface& ed )
    {
        ( void ) ed;
    }
};

//------------------------------------------------------------------------------
// Settings
//------------------------------------------------------------------------------

/**
 * @brief Editor preferences that survive a restart (editor.ini).
 */
struct EditorSettings {
    bool global_xform_gizmo   = false;
    int xform_gizmo_operation = 14463;
    bool draw_wireframe       = false;
    HashMap<String, bool> panel_visible;

    void load( const char* path );
    void save( const char* path ) const;
};

//------------------------------------------------------------------------------
// Editor shell
//------------------------------------------------------------------------------

/**
 * @brief Register editor-owned component types with the world.
 *
 * Flecs refuses to apply traits (EcsCanToggle) to a component once a query
 * references it, so this must run before any editor query or system exists.
 */
void register_editor_component_types( Scene& scene );

/**
 * @brief Owns the frame flow, the panel registry, plugin list and editor state.
 *
 * The engine only sees a handful of ECS systems that forward into this object.
 */
class Editor {
public:
    explicit Editor( Scene& scene [[clang::lifetimebound]] );
    ~Editor();

    Editor( const Editor& )            = delete;
    Editor& operator=( const Editor& ) = delete;

    /**
     * @brief Refresh the per-frame context (scene/services/selection).
     */
    void begin_frame();

    /**
     * @brief Submit the dockspace; must run before any other window.
     */
    void draw_dockspace();

    /**
     * @brief Menu bar + every panel, in (slot, order) order.
     */
    void draw();

    /**
     * @brief Tell the input service whether the UI owns mouse/keyboard.
     */
    void publish_ui_capture();

    /**
     * @brief Resolve the editor camera handle used by the canvas pass.
     */
    RID get_editor_camera();

    bool any_viewport_active() const
    {
        return ctx.any_viewport_hovered || ctx.any_viewport_focused;
    }

    EditorContext& get_ctx()
    {
        return ctx;
    }

    Scene& get_scene() const
    {
        return scene;
    }

    AssetIndex& get_assets()
    {
        return *assets_index;
    }

    AssetImporter& get_importer()
    {
        return *importer;
    }

    EditorSettings& get_settings()
    {
        return settings;
    }

    /**
     * @brief Get panels in draw order (slot, then order).
     */
    const DynamicArray<Ptr<EditorPanel>>& get_panels() const
    {
        return panels;
    }

    /**
     * @brief Release GPU resources the editor owns (render targets, thumbnails).
     */
    void release_gpu();

    void add_plugin( Ptr<EditorPlugin> plugin );

    /** @brief Move staged log lines into the store (render thread). */
    void drain_logs();

    void clear_logs();

    LogStore& logs()
    {
        return log_store;
    }

    // EditorInterface implementation hooks.
    EditorPanel* register_panel( DockSlot slot, int order, Ptr<EditorPanel> panel ) [[clang::lifetimebound]];
    void register_command( StringView menu, StringView label, std::function<void()> on_invoke );
    void register_inspector_handler( Ptr<InspectorHandler> handler );
    void register_property_widget( const rtti::TypeInfo* type, PropertyWidgetFn widget );

private:
    void draw_menu_bar();
    void build_default_layout( unsigned int dockspace_id );
    void apply_settings();
    void sort_panels();

    Scene& scene;
    EditorContext ctx;
    EditorSettings settings;
    Selection selection_state;
    LogStore log_store;
    log::ListenerToken log_token = {};

    Ptr<AssetIndex> assets_index;
    Ptr<AssetImporter> importer;

    DynamicArray<Ptr<EditorPanel>> panels;
    DynamicArray<EditorCommand> commands;
    DynamicArray<Ptr<InspectorHandler>> inspector_handlers;
    DynamicArray<PropertyWidget> property_widgets;
    DynamicArray<Ptr<EditorPlugin>> plugins;
    int active_plugin = -1;

    bool is_layout_checked = false;
    bool is_layout_pending = false;
    bool is_layout_built   = false;
};

//------------------------------------------------------------------------------
// Shared helpers
//------------------------------------------------------------------------------

inline namespace helpers {

inline bool is_internal_component( const rtti::TypeInfo* type )
{
    if (!type)
        return true;
    if (type == rtti::TypeRegistry::find<NodeRefComponent>() || type == rtti::TypeRegistry::find<SelectedTag>())
        return true;
    // editor-only components are not part of the authored scene.
    return type->name && std::strncmp( type->name, "nc::editor", 9 ) == 0;
}

inline bool is_editor_internal_node( Node& node )
{
    return node.has_component<EditorCameraTag>();
}

inline void queue_destroy_subtree( Node& node )
{
    for (auto& child : node.get_children())
        queue_destroy_subtree( child );
    node.destroy();
}

} // namespace helpers

} // namespace nc::editor
