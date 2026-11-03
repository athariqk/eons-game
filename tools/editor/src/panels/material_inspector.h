#pragma once

#include <editor.h>
#include <ncore/runtime/resources/material.h>

namespace nc {
struct MaterialOverrideComponent;
} // namespace nc

namespace nc::editor {

/**
 * @brief Material component inspector for shader uniforms.
 */
class MaterialInspectorHandler final : public InspectorHandler {
public:
    explicit MaterialInspectorHandler();

    bool can_handle( const types::TypeInfo* p_type, void* p_component ) const override;

    void draw( EditorFrame& p_ctx, Node& p_node, const types::TypeInfo* p_type, void* p_component ) override;

private:
    /**
     * @brief Draw the shader's BINDING_SET_MATERIAL uniforms.
     */
    bool draw_uniforms( Material& p_mat );
};

} // namespace nc::editor
