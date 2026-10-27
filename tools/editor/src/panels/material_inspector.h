#pragma once

#include <editor.h>
#include <ncore/resources/resource.h>
#include <ncore/runtime/components/material.h>

namespace nc::editor {

/**
 * @brief Material component inspector for shader uniforms, texture count, etc.
 */
class MaterialInspectorHandler final : public InspectorHandler {
public:
    MaterialInspectorHandler();

    bool can_handle( const rtti::TypeInfo* type, void* component ) const override;

    void draw( EditorContext& ctx, Node& node, const rtti::TypeInfo* type, void* component ) override;

private:
    /**
     * @brief Draw the shader's BINDING_SET_MATERIAL uniforms.
     */
    bool draw_uniforms( MaterialComponent& mat, const MaterialShader& shader );
};

} // namespace nc::editor
