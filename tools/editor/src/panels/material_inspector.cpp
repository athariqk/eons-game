#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include <ncore/runtime/components.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/resources/material.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/video/gfx_types.h>

#include <panels/inspector_panel.h>
#include <panels/material_inspector.h>

namespace nc::editor {

MaterialInspectorHandler::MaterialInspectorHandler() {}

bool MaterialInspectorHandler::can_handle( const types::TypeInfo* p_type, void* p_component ) const
{
    ( void ) p_component;
    return p_type == TypeDB::find<MaterialOverrideComponent>();
}

void MaterialInspectorHandler::draw(
    EditorFrame& p_ctx, Node& p_node, const types::TypeInfo* p_type, void* p_component
)
{
    if (!can_handle( p_type, p_component ))
        return;

    auto& override = *static_cast<MaterialOverrideComponent*>( p_component );

    if (!override.material) {
        ImGui::TextDisabled( "No material assigned" );
        return;
    }

    auto shader = override.material->get_shader();
    if (!shader) {
        ImGui::TextDisabled( "Shader not loaded" );
        return;
    }

    // Uniform writes go into the material's CPU staging block, so the resource
    // is edited in place; every entity referencing it sees the change.
    draw_uniforms( *override.material );
}
//------------------------------------------------------------------------------
// Material shader uniforms
//------------------------------------------------------------------------------

namespace {

int uniform_component_count( gfx::ParameterType p_type )
{
    const uint32_t t = static_cast<uint32_t>( p_type );
    if (t & static_cast<uint32_t>( gfx::ParameterType::VEC4 ))
        return 4;
    if (t & static_cast<uint32_t>( gfx::ParameterType::VEC3 ))
        return 3;
    if (t & static_cast<uint32_t>( gfx::ParameterType::VEC2 ))
        return 2;
    return 1;
}

// Draws one uniform element (scalar or vector). `ptr` points at its bytes.
bool draw_uniform_element( const MaterialParam& p_param, uint8_t* ptr )
{
    const uint32_t type = static_cast<uint32_t>( p_param.type );
    const bool is_float = ( type & static_cast<uint32_t>( gfx::ParameterType::FLOAT_32 ) ) != 0;
    const bool is_unsig = ( type & static_cast<uint32_t>( gfx::ParameterType::UNSIGNED ) ) != 0;
    const int comps     = uniform_component_count( p_param.type );

    if (p_param.widget == gfx::ParamWidget::COLOR && is_float && comps >= 3) {
        auto* f = reinterpret_cast<float*>( ptr );
        return comps == 4 ? ImGui::ColorEdit4( "##v", f, ImGuiColorEditFlags_Float )
                          : ImGui::ColorEdit3( "##v", f, ImGuiColorEditFlags_Float );
    }

    if (p_param.widget == gfx::ParamWidget::SLIDER) {
        bool changed = false;
        if (comps == 1) {
            if (is_float)
                changed |=
                    ImGui::SliderFloat( "##v", reinterpret_cast<float*>( ptr ), p_param.range_min, p_param.range_max );
            else
                changed |= ImGui::SliderInt(
                    "##v", reinterpret_cast<int*>( ptr ), static_cast<int>( p_param.range_min ),
                    static_cast<int>( p_param.range_max )
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
                changed |= ImGui::SliderFloat( "##c", &f[i], p_param.range_min, p_param.range_max );
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

} // namespace

bool MaterialInspectorHandler::draw_uniforms( Material& p_mat )
{
    const auto params = p_mat.get_params();
    if (params.empty())
        return false;

    constexpr ImGuiTableFlags FLAGS =
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoSavedSettings;

    if (!ImGui::BeginTable( "##matparams", 2, FLAGS ))
        return false;
    ImGui::TableSetupColumn( "Property", ImGuiTableColumnFlags_WidthFixed, InspectorPanel::LABEL_WIDTH );
    ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_WidthStretch );

    bool changed = false;
    std::string current_header;

    for (const auto& param : params) {
        if (param.is_texture())
            continue; // textures/samplers are already shown via MaterialOverrideComponent::Textures
        if (!param.element_type)
            continue; // reflected type has no NCORE counterpart

        const std::string full( param.name.data(), param.name.size() );
        const auto dot         = full.rfind( '.' );
        const std::string leaf = ( dot == std::string::npos ) ? full : full.substr( dot + 1 );

        const std::string header( param.header.data(), param.header.size() );
        if (!header.empty() && header != current_header) {
            current_header = header;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex( 0 );
            ImGui::SeparatorText( header.c_str() );
        }

        const uint32_t array_size = param.array_size > 0 ? param.array_size : 1;

        // Edit a packed copy; the material re-coerces and re-strides on upload.
        TypedVar edited = param.value;
        if (edited.count() != array_size)
            edited = TypedVar::array_of( param.element_type, array_size );

        bool field_changed = false;

        ImGui::PushID( full.c_str() );

        if (array_size <= 1) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex( 0 );
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted( leaf.c_str() );
            ImGui::TableSetColumnIndex( 1 );
            ImGui::SetNextItemWidth( -FLT_MIN );
            field_changed |=
                draw_uniform_element( param, static_cast<uint8_t*>( edited.element_ptr( 0 ) ) );
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
                    field_changed |= draw_uniform_element(
                        param, static_cast<uint8_t*>( edited.element_ptr( i ) )
                    );
                    ImGui::PopID();
                }
            }
        }

        ImGui::PopID();

        if (field_changed) {
            p_mat.set_param( StringView( full ), edited );
            changed = true;
        }
    }

    ImGui::EndTable();
    return changed;
}

} // namespace nc::editor
