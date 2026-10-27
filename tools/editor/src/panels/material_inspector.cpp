#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include <ncore/resources/resource.h>
#include <ncore/runtime/components/material.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/video/gfx_types.h>

#include <panels/inspector_panel.h>
#include <panels/material_inspector.h>

namespace nc::editor {

MaterialInspectorHandler::MaterialInspectorHandler() = default;

bool MaterialInspectorHandler::can_handle( const rtti::TypeInfo* type, void* component ) const
{
    ( void ) component;
    return type == rtti::TypeRegistry::find<MaterialComponent>();
}

void MaterialInspectorHandler::draw( EditorContext& ctx, Node& node, const rtti::TypeInfo* type, void* component )
{
    if (!ctx.scene || !can_handle( type, component ))
        return;

    auto& mat = *static_cast<MaterialComponent*>( component );

    // Derived field: keep the bound texture count in sync with the array.
    int count = 0;
    for (int i = 0; i < MaterialComponent::MAX_TEXTURE; ++i) {
        if (mat.textures[i])
            count = i + 1;
    }
    if (mat.texture_count != count) {
        mat.texture_count = count;
        node.notify_changed<MaterialComponent>();
    }

    auto shader = ctx.scene->get_resource_loader().get<MaterialShader>( mat.shader );
    if (!shader) {
        ImGui::TextDisabled( "Shader not loaded" );
        return;
    }

    if (draw_uniforms( mat, *shader )) {
        node.notify_changed<MaterialComponent>();
    }
}
//------------------------------------------------------------------------------
// Material shader uniforms
//------------------------------------------------------------------------------

namespace {

int uniform_component_count( gfx::ParameterType type )
{
    const uint32_t t = static_cast<uint32_t>( type );
    if (t & static_cast<uint32_t>( gfx::ParameterType::VEC4 ))
        return 4;
    if (t & static_cast<uint32_t>( gfx::ParameterType::VEC3 ))
        return 3;
    if (t & static_cast<uint32_t>( gfx::ParameterType::VEC2 ))
        return 2;
    return 1;
}

// Draws one uniform element (scalar or vector). `ptr` points at its bytes.
bool draw_uniform_element( const ShaderParamField& field, uint8_t* ptr )
{
    const uint32_t type = static_cast<uint32_t>( field.type );
    const bool is_float = ( type & static_cast<uint32_t>( gfx::ParameterType::FLOAT_32 ) ) != 0;
    const bool is_unsig = ( type & static_cast<uint32_t>( gfx::ParameterType::UNSIGNED ) ) != 0;
    const int comps     = uniform_component_count( field.type );

    if (field.widget == gfx::ParamWidget::COLOR && is_float && comps >= 3) {
        auto* f = reinterpret_cast<float*>( ptr );
        return comps == 4 ? ImGui::ColorEdit4( "##v", f, ImGuiColorEditFlags_Float )
                          : ImGui::ColorEdit3( "##v", f, ImGuiColorEditFlags_Float );
    }

    if (field.widget == gfx::ParamWidget::SLIDER) {
        bool changed = false;
        if (comps == 1) {
            if (is_float)
                changed |=
                    ImGui::SliderFloat( "##v", reinterpret_cast<float*>( ptr ), field.range_min, field.range_max );
            else
                changed |= ImGui::SliderInt(
                    "##v", reinterpret_cast<int*>( ptr ), static_cast<int>( field.range_min ),
                    static_cast<int>( field.range_max )
                );
            return changed;
        }
        if (is_float) {
            auto* f             = reinterpret_cast<float*>( ptr );
            const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
            const float total   = ImGui::CalcItemWidth();
            const float cell    = ( total - spacing * static_cast<float>( comps - 1 ) ) / static_cast<float>( comps );
            for (int i = 0; i < comps; ++i) {
                if (i)
                    ImGui::SameLine( 0.0f, spacing );
                ImGui::PushID( i );
                ImGui::SetNextItemWidth( cell );
                changed |= ImGui::SliderFloat( "##c", &f[i], field.range_min, field.range_max );
                ImGui::PopID();
            }
        }
        return changed;
    }

    if (is_float) {
        auto* f = reinterpret_cast<float*>( ptr );
        switch (comps) {
            case 4:
                return ImGui::DragFloat4( "##v", f, 0.01f );
            case 3:
                return ImGui::DragFloat3( "##v", f, 0.01f );
            case 2:
                return ImGui::DragFloat2( "##v", f, 0.01f );
            default:
                return ImGui::DragFloat( "##v", f, 0.01f );
        }
    }
    if (is_unsig)
        return ImGui::DragScalar( "##v", ImGuiDataType_U32, ptr, 1.0f );
    return ImGui::DragInt( "##v", reinterpret_cast<int*>( ptr ), 1.0f );
}

const MaterialComponent::Param* find_material_param( const MaterialComponent& mat, StringView leaf )
{
    for (const auto& p : mat.params) {
        if (std::string_view( p.name.data(), p.name.size() ) == leaf)
            return &p;
    }
    return nullptr;
}

} // namespace

bool MaterialInspectorHandler::draw_uniforms( MaterialComponent& mat, const MaterialShader& shader )
{
    // BINDING_SET_MATERIAL == 1 (the g_Material parameter block).
    const auto params = shader.get_param_set( static_cast<gfx::BindingSet>( 1 ) );
    if (params.empty())
        return false;

    constexpr ImGuiTableFlags kFlags =
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoSavedSettings;

    if (!ImGui::BeginTable( "##matparams", 2, kFlags ))
        return false;
    ImGui::TableSetupColumn( "Property", ImGuiTableColumnFlags_WidthFixed, InspectorPanel::kLabelWidth );
    ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_WidthStretch );

    bool changed = false;
    std::string current_header;

    for (const auto& param : params) {
        if (param.resource_type != gfx::ResourceType::CONSTANT_BUFFER)
            continue; // textures/samplers are already shown via MaterialComponent::Textures

        for (const auto& field : param.fields) {
            const std::string header( field.header.data(), field.header.size() );
            if (!header.empty() && header != current_header) {
                current_header = header;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex( 0 );
                ImGui::SeparatorText( header.c_str() );
            }

            const std::string full( field.name.data(), field.name.size() );
            const auto dot         = full.rfind( '.' );
            const std::string leaf = ( dot == std::string::npos ) ? full : full.substr( dot + 1 );

            const uint32_t array_size = field.array_size > 0 ? field.array_size : 1;
            const size_t element_size = array_size > 0 ? ( field.size / array_size ) : field.size;

            std::vector<uint8_t> bytes( field.size, 0 );
            if (const auto* p = find_material_param( mat, leaf )) {
                const size_t n = std::min<size_t>( p->data.size(), bytes.size() );
                std::memcpy( bytes.data(), p->data.data(), n );
            }

            bool field_changed = false;

            ImGui::PushID( full.c_str() );

            if (array_size <= 1) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex( 0 );
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted( leaf.c_str() );
                ImGui::TableSetColumnIndex( 1 );
                ImGui::SetNextItemWidth( -FLT_MIN );
                field_changed |= draw_uniform_element( field, bytes.data() );
            } else {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex( 0 );
                bool open = ImGui::TreeNodeEx(
                    leaf.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                      ImGuiTreeNodeFlags_DefaultOpen
                );
                ImGui::TableSetColumnIndex( 1 );
                ImGui::TextDisabled( "%u elements", array_size );
                if (open) {
                    for (uint32_t i = 0; i < array_size; ++i) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex( 0 );
                        ImGui::Text( "[%u]", i );
                        ImGui::TableSetColumnIndex( 1 );
                        ImGui::SetNextItemWidth( -FLT_MIN );
                        ImGui::PushID( static_cast<int>( i ) );
                        field_changed |= draw_uniform_element( field, bytes.data() + i * element_size );
                        ImGui::PopID();
                    }
                }
            }

            ImGui::PopID();

            if (field_changed) {
                mat.set_param( StringView( leaf ), bytes.data(), bytes.size() );
                changed = true;
            }
        }
    }

    ImGui::EndTable();
    return changed;
}

} // namespace nc::editor
