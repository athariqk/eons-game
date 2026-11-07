// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: editor_panel.h

#pragma once

#include <cstdint>

#include <imgui.h>

#include <editor/editor_frame.h>
#include <ncore/core/collection.h>

namespace nc {
class RenderService;
}

namespace sea::editor {

/**
 * @brief Named dock destinations.
 */
enum class DockSlot : uint8_t {
    TOP,
    LEFT_UPPER,
    LEFT_LOWER,
    RIGHT_UPPER,
    RIGHT_LOWER,
    BOTTOM,
    CENTER,
    FLOATING,
};

/**
 * @brief One editor window.
 *
 * Panels are ordinary objects: the editor owns them, draws them in (slot, order)
 * order and builds the dock layout from them. A panel takes its stable
 * dependencies in the constructor and reads only per-frame state from
 * EditorFrame.
 */
class EditorPanel {
public:
    EditorPanel(
        const nc::String& p_title, DockSlot p_slot, int p_order = 0, bool p_visible = true, bool p_closable = true
    ) : title( p_title ), slot( p_slot ), order( p_order ), is_visible( p_visible ), is_closable( p_closable )
    {}

    virtual ~EditorPanel() = default;

    EditorPanel( const EditorPanel& )            = delete;
    EditorPanel& operator=( const EditorPanel& ) = delete;

    const nc::String& get_title() const
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
    void draw_window( EditorFrame& frame );

protected:
    /**
     * @brief Content submitted between Begin and End (only when open).
     */
    virtual void draw_contents( EditorFrame& frame ) = 0;

    /**
     * @brief Window flags merged into the Begin call.
     */
    virtual ImGuiWindowFlags window_flags() const
    {
        return ImGuiWindowFlags_None;
    }

    /**
     * @brief Run before Begin (window class, style vars).
     */
    virtual void pre_begin( EditorFrame& ) {}

    /**
     * @brief Run after End when the window is collapsed/closed.
     */
    virtual void on_collapsed( EditorFrame& ) {}

    /**
     * @brief Run after End (pop style vars, modal windows).
     */
    virtual void post_end( EditorFrame& ) {}

    nc::String title;
    DockSlot slot;
    int order;
    bool is_visible;
    bool is_closable;
};

} // namespace sea::editor
