// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: editor.h

#pragma once

// clang-format off
#include <imgui_internal.h>
#include <ImGuizmo.h> // must come after imgui includes
// clang-format on

#include <cstdint>

#include <ncore/core/memory.h>
#include <ncore/utils/log.h>

#include "editor_frame.h"
#include "editor_selection.h"
#include "inspector/property_drawer_db.h"
#include "io/editor_assets.h"
#include "panels/editor_panel.h"

namespace nc {
class AssetService;
class EcsWorld;
class InputService;
class Node;
class RenderService;
class ResourceLoader;
class Scene;
struct TimeComponent;
} // namespace nc

namespace sea::editor {

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
    nc::DynamicArray<nc::String> channels;

    bool is_level_enabled[6] = { false, false, false, true, true, true };
    int channel_filter       = -1;
    bool is_autoscroll       = true;

    void clear();

    static ImVec4 level_color( int level );
};

/**
 * @brief Editor preferences that survive a restart (editor.ini).
 */
struct EditorSettings {
    bool global_xform_gizmo   = false;
    int xform_gizmo_operation = ImGuizmo::OPERATION::UNIVERSAL;
    bool draw_wireframe       = false;
    nc::HashMap<nc::String, bool> panel_visible;

    void load( const char* path );
    void save( const char* path ) const;
};

/**
 * @brief Owns the frame flow, the panel registry and editor-wide state.
 *
 * The engine only sees a handful of ECS systems that forward into this object.
 * Panels are constructed by the composition root (editor_factory) and take their
 * stable dependencies directly; Editor itself never resolves services except
 * for input capture, which is shell-wide.
 */
class Editor : public nc::Object {
    NCLASS( Editor, nc::Object )

public:
    Editor( nc::Scene& scene NC_LIFETIME_BOUND, nc::AssetService* compiler );
    ~Editor() override;

    Editor( const Editor& )            = delete;
    Editor& operator=( const Editor& ) = delete;

    static constexpr const char* DEFAULT_SETTINGS_PATH = "editor.ini";

    /**
     * @brief Refresh the per-frame frame state (scene/selection/assets).
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

    void apply_settings();

    /**
     * @brief Release panel GPU state and stop the import worker.
     *
     * Must run while the renderer and scene are still alive.
     */
    void shutdown( nc::RenderService& renderer );

    bool any_viewport_active() const
    {
        return current_frame.any_viewport_hovered || current_frame.any_viewport_focused;
    }

    EditorFrame& get_frame()
    {
        return current_frame;
    }

    nc::Scene& get_scene() const
    {
        return scene;
    }

    EditorSettings& get_settings()
    {
        return settings;
    }

    EditorSelection& get_selection()
    {
        return selection_state;
    }

    EditorAssets& get_assets()
    {
        return asset_system;
    }

    const EditorAssets& get_assets() const
    {
        return asset_system;
    }

    /**
     * @brief Get panels in draw order (slot, then order).
     */
    const nc::DynamicArray<nc::Ptr<EditorPanel>>& get_panels() const
    {
        return panels;
    }

    PropertyDrawerDB& get_property_drawers() noexcept
    {
        return property_drawers;
    }

    /**
     * @brief Move staged log lines into the store (render thread).
     */
    void drain_logs();

    void clear_logs();

    LogStore& get_logs()
    {
        return log_store;
    }

    // Panel / contribution registration.
    EditorPanel* register_panel( nc::Ptr<EditorPanel> panel ) NC_LIFETIME_BOUND;
    void sort_panels();

    void register_drawer( const nc::types::TypeInfo* type, PropertyDrawerFn drawer );
    bool unregister_drawer( const nc::types::TypeInfo* type );

private:
    void draw_menu_bar();
    void build_default_layout( unsigned int dockspace_id );

    nc::Scene& scene;
    EditorFrame current_frame;
    EditorSettings settings;
    EditorSelection selection_state;
    EditorAssets asset_system;
    LogStore log_store;
    nc::log::ListenerToken log_token = {};

    nc::DynamicArray<nc::Ptr<EditorPanel>> panels;
    PropertyDrawerDB property_drawers;

    bool is_layout_checked = false;
    bool is_layout_pending = false;
    bool is_layout_built   = false;
    bool is_shutdown       = false;
};

} // namespace sea::editor
