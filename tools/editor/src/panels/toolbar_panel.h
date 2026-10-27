#pragma once

#include <editor.h>

namespace nc::editor {

/**
 * @brief Toolbar strip: gizmo mode, world/local, wireframe.
 */
class ToolbarPanel final : public EditorPanel {
public:
    ToolbarPanel();

    void draw( EditorContext& ctx ) override;
};

} // namespace nc::editor
