#pragma once

#include "editor_panel.h"

namespace nc {
class Scene;
}

namespace sea::editor {

struct EditorSettings;

/**
 * @brief Toolbar strip for showing gizmo mode, world/local, wireframe checkbuttons etc.
 */
class ToolbarPanel final : public EditorPanel {
public:
    ToolbarPanel( nc::Scene& p_scene NC_LIFETIME_BOUND, EditorSettings& p_settings NC_LIFETIME_BOUND );

    void draw_contents( EditorFrame& frame ) override;

    ImGuiWindowFlags window_flags() const override;
    void pre_begin( EditorFrame& ) override;

private:
    nc::Scene& scene;
    EditorSettings& settings;
};

} // namespace sea::editor
