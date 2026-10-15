#pragma once

#include <ncore/core/types.h>

namespace nc {
struct MaterialComponent;
class MaterialShader;
} // namespace nc

namespace nc::editor {
namespace propgrid {

using namespace nc::rtti;

inline constexpr float kLabelWidth = 140.0f;
inline constexpr float kIndentPx   = 12.0f;

bool draw_component_properties( const TypeInfo* type, void* data );

/**
 * @brief Draw a material's shader uniforms (BINDING_SET_MATERIAL) as an inspector
 * section, reading/writing MaterialComponent::Params. Widget kind comes from the
 * shader's reflected [Color]/[Range]/[Header] attributes.
 *
 * @return true if any value changed.
 */
bool draw_material_uniforms( MaterialComponent& mat, const MaterialShader& shader );

} // namespace propgrid
} // namespace nc::editor
