#pragma once

#include <ncore/runtime/resources/material.h>

#include <editor/inspector/inspector_plugin.h>

namespace sea::editor {
class PropertyGrid;
} // namespace sea::editor

namespace sea::editor {

class EditorAssets;

/**
 * @brief Material component inspector for shader uniforms and textures.
 * Immediate feedback, sections, color pickers, drag-drop textures. Godot-inspired but
 * keeps our Ref + RTTI model.
 */
class MaterialInspectorPlugin final : public InspectorPlugin {
public:
    explicit MaterialInspectorPlugin( EditorAssets& p_assets NC_LIFETIME_BOUND );

    bool can_handle( const nc::types::TypeInfo* type, void* instance ) const override;

    bool replaces_default() const override;

    void parse_begin( EditorFrame& frame, const nc::types::TypeInfo* type, void* instance ) override;

    void parse_end( EditorFrame& frame, const nc::types::TypeInfo* type, void* instance ) override;

private:
    EditorAssets& assets;
};

/**
 * @brief Draw the shader-driven uniform + texture parameter editor for a Material.
 * Used by both the dedicated inspector plugin and from inside the generic
 * PropertyGrid when a Material (or Ref to one) is encountered.
 */
void draw_material_params( EditorAssets& assets, nc::Material& mat );

/**
 * @brief Grid-integrated version: emits rows using the caller's PropertyGrid
 * table so the params appear *inside* the expanded Material record section
 * (matching the Godot "shader params within the material resource expansion" feel).
 */
void draw_material_params_grid( PropertyGrid& grid, EditorAssets& assets, nc::Material& mat, int base_depth );

} // namespace sea::editor
