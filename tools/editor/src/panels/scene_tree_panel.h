#pragma once

#include <editor.h>
#include <ncore/runtime/node.h>

namespace nc::editor {

/**
 * @brief Scene hierarchy for node selection, reparenting, spawn/delete and rename.
 */
class SceneTreePanel final : public EditorPanel {
public:
    SceneTreePanel(
        Scene& p_scene [[clang::lifetimebound]], EditorSelection& p_selection [[clang::lifetimebound]],
        AssetDatabase& p_assets [[clang::lifetimebound]], ResourceLoader& p_loader [[clang::lifetimebound]]
    );

    void draw_contents( EditorFrame& ctx ) override;

    void post_end( EditorFrame& ctx ) override;

private:
    void queue_destroy_subtree( Node& node );
    void draw_node( Node& node, EditorFrame& ctx );
    void draw_rename_modal( EditorFrame& ctx );

    Scene& scene;
    EditorSelection& selection;
    AssetDatabase& assets;
    ResourceLoader& loader;

    EcsEntity rename_entity = INVALID_ENTITY_ID;
    char rename_buf[256]    = {};
    bool show_rename        = false;
};

} // namespace nc::editor
