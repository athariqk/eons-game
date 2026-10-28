#pragma once

// clang-format off
#include <imgui_internal.h>
#include <ImGuizmo.h> // must come after imgui includes
// clang-format on

#include <cstdint>
#include <functional>
#include <memory>

#include <imgui.h>

#include <ncore/core/collection.h>
#include <ncore/core/memory.h>
#include <ncore/core/types.h>
#include <ncore/utils/log.h>

#include "editor_selection.h"

namespace nc {
class EcsWorld;
class InputService;
class Node;
class RenderService;
class ResourceLoader;
class Scene;
struct TimeComponent;

namespace editor {
class AssetDatabase;
class AssetImporter;
class ThumbnailCache;
} // namespace editor
} // namespace nc

namespace nc::editor {

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

class InspectorHandler;
struct PropertyWidget;

/**
 * @brief Per-frame state a panel reads while drawing.
 */
struct EditorFrame {
    Scene* scene = nullptr;

    // Published by the viewport panels while drawing.
    bool any_viewport_hovered = false;
    bool any_viewport_focused = false;
};

//------------------------------------------------------------------------------
// Inspector extension points
//------------------------------------------------------------------------------

/** @brief Layout constants shared by the inspector sheet and its handlers. */
inline constexpr float INSPECTOR_LABEL_WIDTH = 140.0f;
inline constexpr float INSPECTOR_INDENT_PX   = 12.0f;

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

    virtual void draw( EditorFrame& ctx, Node& node, const rtti::TypeInfo* type, void* component ) = 0;

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
    EditorFrame& ctx, const rtti::RecordInfo* owner, StringView field, const rtti::TypeInfo* type, void* value,
    bool& out_changed
)>;

struct PropertyWidget {
    const rtti::TypeInfo* Type = nullptr;
    PropertyWidgetFn Draw;
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
     * @brief Draw the panel window: Begin/End plus the panel's contents.
     *
     * The shell calls this every frame. It owns the ImGui window so concrete
     * panels only implement draw_contents() and the optional lifecycle hooks.
     */
    void draw_window( EditorFrame& ctx );

    /**
     * @brief Release GPU resources owned by the panel (render targets, thumbnails).
     */
    virtual void release_gpu( RenderService& renderer )
    {
        ( void ) renderer;
    }

protected:
    /** @brief Content submitted between Begin and End (only when open). */
    virtual void draw_contents( EditorFrame& ctx ) = 0;

    /** @brief Window flags merged into the Begin call. */
    virtual ImGuiWindowFlags window_flags() const
    {
        return ImGuiWindowFlags_None;
    }

    /** @brief Run before Begin (window class, style vars). */
    virtual void pre_begin( EditorFrame& ) {}

    /** @brief Run after End when the window is collapsed/closed. */
    virtual void on_collapsed( EditorFrame& ) {}

    /** @brief Run after End (pop style vars, modal windows). */
    virtual void post_end( EditorFrame& ) {}

    String title;
    DockSlot slot;
    int order;
    bool is_visible;
    bool is_closable;
};

//------------------------------------------------------------------------------
// Settings
//------------------------------------------------------------------------------

/**
 * @brief Editor preferences that survive a restart (editor.ini).
 */
struct EditorSettings {
    bool global_xform_gizmo   = false;
    int xform_gizmo_operation = ImGuizmo::OPERATION::UNIVERSAL;
    bool draw_wireframe       = false;
    HashMap<String, bool> panel_visible;

    void load( const char* path );
    void save( const char* path ) const;
};

//------------------------------------------------------------------------------
// Editor shell
//------------------------------------------------------------------------------

/**
 * @brief Owns the frame flow, the panel registry and editor state.
 *
 * The engine only sees a handful of ECS systems that forward into this object.
 */
class Editor : public Object {
    NCLASS( Editor, Object )

public:
    Editor( Scene& scene [[clang::lifetimebound]] );
    ~Editor() override;

    Editor( const Editor& )            = delete;
    Editor& operator=( const Editor& ) = delete;

    /**
     * @brief Refresh the per-frame context (scene/services/selection).
     */
    void begin_frame();

    /**
     * @brief Submit the dockspace. Must run before any other window.
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

    bool any_viewport_active() const
    {
        return frame.any_viewport_hovered || frame.any_viewport_focused;
    }

    EditorFrame& get_frame()
    {
        return frame;
    }

    Scene& get_scene() const
    {
        return scene;
    }

    AssetDatabase& get_assets()
    {
        return *assets_database;
    }

    AssetImporter& get_importer()
    {
        return *importer;
    }

    ThumbnailCache& get_thumbnails()
    {
        return *thumbnails;
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

    /** @brief Move staged log lines into the store (render thread). */
    void drain_logs();

    void clear_logs();

    LogStore& logs()
    {
        return log_store;
    }

    // Panel / contribution registration.
    EditorPanel* register_panel( DockSlot slot, int order, Ptr<EditorPanel> panel ) [[clang::lifetimebound]];
    void register_inspector_handler( Ptr<InspectorHandler> handler );
    void register_property_widget( const rtti::TypeInfo* type, PropertyWidgetFn widget );

private:
    void create_panels();
    void draw_menu_bar();
    void build_default_layout( unsigned int dockspace_id );
    void apply_settings();
    void sort_panels();

    Scene& scene;
    EditorFrame frame;
    EditorSettings settings;
    EditorSelection selection_state;
    LogStore log_store;
    log::ListenerToken log_token = {};

    Ptr<AssetDatabase> assets_database;
    Ptr<AssetImporter> importer;
    Ptr<ThumbnailCache> thumbnails;

    DynamicArray<Ptr<EditorPanel>> panels;
    DynamicArray<Ptr<InspectorHandler>> inspector_handlers;
    DynamicArray<PropertyWidget> property_widgets;

    bool is_layout_checked = false;
    bool is_layout_pending = false;
    bool is_layout_built   = false;
};

} // namespace nc::editor
