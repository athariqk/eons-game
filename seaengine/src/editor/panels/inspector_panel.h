// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: inspector_panel.h

#pragma once

#include <ncore/core/memory.h>
#include <ncore/types/typeinfo.h>

#include "editor_panel.h"

#include <editor/inspector/inspector_plugin.h>
#include <editor/inspector/property_drawer_db.h>

namespace nc {
class Node;
class Scene;
} // namespace nc

namespace sea::editor {

class EditorAssets;
class EditorSelection;

/**
 * @brief Property grid for editor selection.
 */
class InspectorPanel final : public EditorPanel {
public:
    InspectorPanel(
        nc::Scene& p_scene NC_LIFETIME_BOUND, EditorSelection& p_selection NC_LIFETIME_BOUND,
        EditorAssets& p_assets NC_LIFETIME_BOUND, PropertyDrawerDB& p_drawers NC_LIFETIME_BOUND
    );

    void add_plugin( nc::Ptr<InspectorPlugin> p_plugin );

    void draw_contents( EditorFrame& frame ) override;

private:
    /**
     * @brief Draws the inspector for a selected node and its components.
     */
    void draw_node( EditorFrame& p_frame, nc::Node& p_selected );
    void draw_node_header( nc::Node& selected );
    void draw_add_component( nc::Node& selected );

    /**
     * @brief Inspector body for a selected asset (browser selection).
     */
    void draw_asset( EditorFrame& frame, nc::StringView p_path );

    /**
     * @brief Plugins' parse_begin, the generic grid (unless a plugin replaces it), then parse_end.
     * @return true if the generic grid modified the value.
     */
    bool draw_body( EditorFrame& frame, const nc::types::TypeInfo* p_type, void* p_data );

    nc::Scene& scene;
    EditorSelection& selection;
    EditorAssets& assets;
    const PropertyDrawerDB& property_drawers;

    nc::DynamicArray<nc::Ptr<InspectorPlugin>> plugins;

    char component_filter[64] = {};
    int add_cursor            = 0;

    nc::TypedVar clipboard; // copied component values (Copy/Paste Values)

    nc::BumpAllocator<const nc::types::TypeInfo*> to_remove;
};

} // namespace sea::editor
