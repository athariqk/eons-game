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
    explicit MaterialInspectorHandler( Scene& scene [[clang::lifetimebound]] );

    bool can_handle( const rtti::TypeInfo* p_type, void* p_component ) const override;

    void draw( EditorFrame& p_ctx, Node& p_node, const rtti::TypeInfo* p_type, void* p_component ) override;

private:
    /**
     * @brief Draw the shader's BINDING_SET_MATERIAL uniforms.
     */
    bool draw_uniforms( MaterialComponent& p_mat, const MaterialShader& p_shader );

    Scene& scene;
};

} // namespace nc::editor
