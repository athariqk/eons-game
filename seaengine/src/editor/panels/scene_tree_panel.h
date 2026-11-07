#pragma once

#include <ncore/runtime/node.h>

#include "editor_panel.h"

namespace nc {
class Scene;
}

namespace sea::editor {

class EditorAssets;
class EditorSelection;
class EditorWorldHost;

/**
 * @brief Scene hierarchy for node selection, reparenting, spawn/delete and rename.
 */
class SceneTreePanel final : public EditorPanel {
public:
    SceneTreePanel(
        nc::Scene& p_scene NC_LIFETIME_BOUND, EditorSelection& p_selection NC_LIFETIME_BOUND,
        EditorAssets& p_assets NC_LIFETIME_BOUND
    );

    void draw_contents( EditorFrame& frame ) override;

    void post_end( EditorFrame& frame ) override;

private:
    void queue_destroy_subtree( nc::Node& node );
    void draw_node( nc::Node& node, EditorFrame& frame );
    void draw_rename_modal( EditorFrame& frame );

    nc::Scene& scene;
    EditorSelection& selection;
    EditorAssets& assets;

    nc::EcsEntity rename_entity = nc::INVALID_ENTITY_ID;
    char rename_buf[256]        = {};
    bool show_rename            = false;
};

} // namespace sea::editor
