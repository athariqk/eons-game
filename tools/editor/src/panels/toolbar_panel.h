#pragma once

#include <editor.h>

namespace nc::editor {

/**
 * @brief Toolbar strip for showing gizmo mode, world/local, wireframe checkbuttons etc.
 */
class ToolbarPanel final : public EditorPanel {
public:
    explicit ToolbarPanel( EditorSettings& p_settings NC_LIFETIME_BOUND );

    void draw_contents( EditorFrame& ctx ) override;

    ImGuiWindowFlags window_flags() const override;
    void pre_begin( EditorFrame& ) override;

private:
    EditorSettings& settings;
};

} // namespace nc::editor
