#pragma once

#include <editor.h>
#include <ncore/runtime/node.h>

namespace nc::editor {

/**
 * @brief Scene hierarchy: selection, reparenting, spawn/delete, rename modal.
 */
class SceneTreePanel final : public EditorPanel {
public:
    SceneTreePanel();

    void draw( EditorContext& ctx ) override;

private:
    void draw_node( Node& node, EditorContext& ctx );

    void draw_rename_modal( EditorContext& ctx );

    EcsEntity rename_entity_ = INVALID_ENTITY_ID;
    char rename_buf_[256]    = {};
    bool show_rename_        = false;
};

} // namespace nc::editor
