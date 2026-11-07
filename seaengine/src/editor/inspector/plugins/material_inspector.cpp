#include "material_inspector.h"

#include <imgui.h>

#include <ncore/runtime/resources/material.h>
#include <ncore/services/video/gfx_types.h>

#include <editor/inspector/property_grid.h>
#include <editor/io/editor_assets.h>

namespace sea::editor {

MaterialInspectorPlugin::MaterialInspectorPlugin( EditorAssets& p_assets ) : assets( p_assets ) {}

bool MaterialInspectorPlugin::can_handle( const nc::types::TypeInfo* p_type, void* p_instance ) const
{
    if (!p_type || !p_instance || !p_type->is_record())
        return false;

    // Type-only check: safe for both static Material and most-derived class_info
    // from Object::get_class_info(). Never cast arbitrary component blobs to Object*.
    const nc::types::RecordInfo* mat = nc::TypeDB::find_record<nc::Material>();
    if (!mat)
        return false;
    return nc::TypeDB::is_derived_or_same( static_cast<const nc::types::RecordInfo*>( p_type ), mat );
}

bool MaterialInspectorPlugin::replaces_default() const
{
    return true; // When a Material is the thing being inspected, show only the nice shader params UI
}

namespace {

int uniform_component_count( nc::gfx::ParameterType p_type )
{
    const uint32_t t = static_cast<uint32_t>( p_type );
    if (t & static_cast<uint32_t>( nc::gfx::ParameterType::VEC4 ))
        return 4;
    if (t & static_cast<uint32_t>( nc::gfx::ParameterType::VEC3 ))
        return 3;
    if (t & static_cast<uint32_t>( nc::gfx::ParameterType::VEC2 ))
        return 2;
    return 1;
}

bool draw_uniform_element( const nc::MaterialParam& p_param, uint8_t* ptr )
{
    const uint32_t type = static_cast<uint32_t>( p_param.type );
    const bool is_float = ( type & static_cast<uint32_t>( nc::gfx::ParameterType::FLOAT_32 ) ) != 0;
    const bool is_unsig = ( type & static_cast<uint32_t>( nc::gfx::ParameterType::UNSIGNED ) ) != 0;
    const int comps     = uniform_component_count( p_param.type );

    if (p_param.widget == nc::gfx::ParamWidget::COLOR && is_float && comps >= 3) {
        auto* f = reinterpret_cast<float*>( ptr );
        return comps == 4 ? ImGui::ColorEdit4( "##v", f, ImGuiColorEditFlags_Float )
                          : ImGui::ColorEdit3( "##v", f, ImGuiColorEditFlags_Float );
    }

    if (p_param.widget == nc::gfx::ParamWidget::SLIDER) {
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

namespace {

const char* file_part( const char* path )
{
    const char* s = std::strrchr( path, '/' );
    if (!s)
        s = std::strrchr( path, '\\' );
    return s ? s + 1 : path;
}

const AssetEntry* find_asset_entry_for_rid( EditorAssets& assets, nc::RID rid )
{
    if (!rid.is_valid())
        return nullptr;
    for (const auto& e : assets.get_entries()) {
        if (e.kind == AssetKind::IMAGE) {
            nc::RID cand = assets.ensure( e.path );
            if (cand == rid)
                return &e;
        }
    }
    return nullptr;
}

void draw_texture_thumbnail( EditorAssets& assets, nc::RID rid )
{
    if (!rid.is_valid())
        return;
    if (const AssetEntry* e = find_asset_entry_for_rid( assets, rid ); e) {
        if (nc::RID thumb = assets.thumbnail( e->path ); thumb.is_valid()) {
            ImGui::Image( reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( thumb.value ) ), ImVec2( 22, 22 ) );
            ImGui::SameLine( 0.0f, 4.0f );
        }
    }
}

ImVec4 sample_material_preview_color( const nc::Material& mat )
{
    ImVec4 col{ 0.25f, 0.55f, 0.85f, 1.0f };
    const auto params = mat.get_params();
    for (const auto& p : params) {
        if (p.is_texture() || !p.element_type || !p.value.has_value())
            continue;

        std::string n( p.name.data(), p.name.size() );
        for (char& c : n)
            c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );

        if (n.find( "albedo" ) != std::string::npos || n.find( "basecolor" ) != std::string::npos ||
            n.find( "base_color" ) != std::string::npos || n.find( "diffuse" ) != std::string::npos ||
            n.find( "color" ) != std::string::npos || n.find( "tint" ) != std::string::npos) {

            if (p.value_count() >= 3) {
                const float* f = static_cast<const float*>( p.value.element_ptr( 0 ) );
                col.x          = f[0];
                col.y          = f[1];
                col.z          = f[2];
                if (p.value_count() >= 4)
                    col.w = f[3];
                break;
            }
        }
    }
    return col;
}

} // namespace

// Reusable drawing function for the shader param editor (uniforms + textures).
// Uses manual label + control positioning forced from the window content region
// (GetWindowContentRegionMin().x + LABEL_WIDTH) so labels/values always align
// with the primary inspector columns even when invoked from inside a nested
// record expansion (e.g. Ref<Material> inside any component's PropertyGrid table)
// or from a component card. No inner tables, no per-component hardcode.
void draw_material_params( EditorAssets& assets, nc::Material& mat )
{
    auto shader = mat.get_shader();
    if (!shader) {
        ImGui::TextDisabled( "No MaterialShader" );
        return;
    }

    const auto params = mat.get_params();
    if (params.empty()) {
        ImGui::TextDisabled( "Shader has no exposed parameters" );
        return;
    }

    ImGui::SeparatorText( "Shader Parameters" );

    std::string current_header;

    for (const auto& param : params) {
        if (!param.element_type && !param.is_texture())
            continue;

        const std::string full( param.name.data(), param.name.size() );
        const auto dot         = full.rfind( '.' );
        const std::string leaf = ( dot == std::string::npos ) ? full : full.substr( dot + 1 );

        const std::string header( param.header.data(), param.header.size() );
        if (!header.empty() && header != current_header) {
            current_header = header;
            ImGui::SeparatorText( header.c_str() );
        }

        ImGui::PushID( full.c_str() );

        const float content_min_x = ImGui::GetWindowContentRegionMin().x;
        const float label_x       = content_min_x;
        const float value_x       = content_min_x + PropertyGrid::LABEL_WIDTH;

        bool field_changed = false;
        nc::TypedVar edited;
        uint32_t array_size = 1;
        if (!param.is_texture()) {
            array_size = param.array_size > 0 ? param.array_size : 1;
            edited     = param.value;
            if (edited.count() != array_size)
                edited = nc::TypedVar::array_of( param.element_type, array_size );
        }
        const bool is_array = !param.is_texture() && array_size > 1;

        ImGui::SetCursorPosX( label_x );
        ImGui::AlignTextToFramePadding();

        if (is_array) {
            bool open = ImGui::TreeNodeEx(
                leaf.c_str(),
                ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_DefaultOpen
            );
            ImGui::SameLine( value_x );
            ImGui::TextDisabled( "(%u)", array_size );
            if (open) {
                for (uint32_t i = 0; i < array_size; ++i) {
                    ImGui::PushID( static_cast<int>( i ) );
                    ImGui::SetCursorPosX( label_x + PropertyGrid::INDENT_PX );
                    ImGui::Text( "[%u]", i );
                    ImGui::SameLine( value_x );
                    ImGui::SetNextItemWidth( -FLT_MIN );
                    field_changed |= draw_uniform_element( param, static_cast<uint8_t*>( edited.element_ptr( i ) ) );
                    ImGui::PopID();
                }
            }
        } else if (param.is_texture()) {
            ImGui::TextUnformatted( leaf.c_str() );
            ImGui::SameLine( value_x );
            // Texture control (simplified); show friendly leaf name when possible
            nc::RID current = param.texture_rid;
            const char* btn = current.is_valid() ? "..." : "(none)";

            if (current.is_valid()) {
                draw_texture_thumbnail( assets, current );
                if (const AssetEntry* e = find_asset_entry_for_rid( assets, current ); e) {
                    ImGui::TextUnformatted( file_part( e->path.c_str() ) );
                } else {
                    ImGui::Text( "nc::RID:%llu", static_cast<unsigned long long>( current.value ) );
                }
                ImGui::SameLine();
            }

            if (ImGui::Button( btn )) {
                // future: asset picker
            }

            if (ImGui::BeginDragDropTarget()) {
                const ImGuiPayload* payload = nullptr;
                if (( payload = ImGui::AcceptDragDropPayload( "ASSET_PATH" ) ) ||
                    ( payload = ImGui::AcceptDragDropPayload( PropertyGrid::ASSET_PAYLOAD ) )) {
                    const char* path = static_cast<const char*>( payload->Data );
                    if (const AssetEntry* e = assets.find( path ); e && e->kind == AssetKind::IMAGE) {
                        nc::RID new_tex = assets.ensure( path );
                        if (new_tex.is_valid()) {
                            nc::TypedVar tv( new_tex );
                            mat.set_param( nc::StringView( full ), tv );
                            field_changed = true;
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (ImGui::BeginPopupContextItem( "##texctx" )) {
                if (ImGui::MenuItem( "Clear" )) {
                    mat.set_param( nc::StringView( full ), nc::TypedVar() );
                    field_changed = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        } else {
            // scalar uniform
            ImGui::TextUnformatted( leaf.c_str() );
            ImGui::SameLine( value_x );
            ImGui::SetNextItemWidth( -FLT_MIN );
            field_changed |= draw_uniform_element( param, static_cast<uint8_t*>( edited.element_ptr( 0 ) ) );
        }

        if (!param.is_texture() && field_changed) {
            mat.set_param( nc::StringView( full ), edited );
        }

        ImGui::PopID();
    }
}

// Grid-integrated version: the params become proper rows inside the Material's
// record expansion (so they appear "within the section that expands when you
// click the material resource", exactly like the Godot behavior the user liked).
void draw_material_params_grid( PropertyGrid& grid, EditorAssets& assets, nc::Material& mat, int base_depth )
{
    auto shader = mat.get_shader();
    if (!shader) {
        grid.begin_prop_row( "Shader", base_depth );
        ImGui::TextDisabled( "No MaterialShader" );
        return;
    }

    const auto params = mat.get_params();
    if (params.empty()) {
        grid.begin_prop_row( "Params", base_depth );
        ImGui::TextDisabled( "Shader has no exposed parameters" );
        return;
    }

    // Section header as a full-width row under the current material record
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex( 0 );
    ImGui::SeparatorText( "Shader Parameters" );
    ImGui::TableSetColumnIndex( 1 );
    ImGui::TextDisabled( "" );

    // Compact live preview inside the material record expansion
    grid.begin_prop_row( "Preview", base_depth + 1 );
    const ImVec4 preview_col = sample_material_preview_color( mat );
    ImGui::ColorButton(
        "##gprev", preview_col, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2( 20, 20 )
    );

    std::string current_header;

    for (const auto& param : params) {
        if (!param.element_type && !param.is_texture())
            continue;

        const std::string full( param.name.data(), param.name.size() );
        const auto dot         = full.rfind( '.' );
        const std::string leaf = ( dot == std::string::npos ) ? full : full.substr( dot + 1 );

        const std::string header( param.header.data(), param.header.size() );
        if (!header.empty() && header != current_header) {
            current_header = header;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex( 0 );
            ImGui::SeparatorText( header.c_str() );
            ImGui::TableSetColumnIndex( 1 );
        }

        ImGui::PushID( full.c_str() );

        const uint32_t array_size = param.array_size > 0 ? param.array_size : 1;
        const bool is_array       = !param.is_texture() && array_size > 1;

        bool field_changed = false;
        nc::TypedVar edited;
        if (!param.is_texture()) {
            edited = param.value;
            if (edited.count() != array_size)
                edited = nc::TypedVar::array_of( param.element_type, array_size );
        }

        if (is_array) {
            const bool open = grid.begin_tree_row( leaf.c_str(), base_depth + 1, ImGuiTreeNodeFlags_DefaultOpen );
            ImGui::TextDisabled( "(%u)", array_size );
            if (open) {
                for (uint32_t i = 0; i < array_size; ++i) {
                    char idx_label[16];
                    std::snprintf( idx_label, sizeof idx_label, "[%u]", i );
                    grid.begin_prop_row( idx_label, base_depth + 2 );
                    ImGui::SetNextItemWidth( -FLT_MIN );
                    field_changed |= draw_uniform_element( param, static_cast<uint8_t*>( edited.element_ptr( i ) ) );
                }
            }
        } else {
            grid.begin_prop_row( leaf.c_str(), base_depth + 1 );

            if (param.is_texture()) {
                nc::RID current = param.texture_rid;
                const char* btn = current.is_valid() ? "..." : "(none)";

                if (current.is_valid()) {
                    draw_texture_thumbnail( assets, current );
                    if (const AssetEntry* e = find_asset_entry_for_rid( assets, current ); e) {
                        ImGui::TextUnformatted( file_part( e->path.c_str() ) );
                    } else {
                        ImGui::Text( "nc::RID:%llu", static_cast<unsigned long long>( current.value ) );
                    }
                    ImGui::SameLine();
                }

                if (ImGui::Button( btn )) {
                    // future picker
                }

                if (ImGui::BeginDragDropTarget()) {
                    const ImGuiPayload* payload = nullptr;
                    if (( payload = ImGui::AcceptDragDropPayload( "ASSET_PATH" ) ) ||
                        ( payload = ImGui::AcceptDragDropPayload( PropertyGrid::ASSET_PAYLOAD ) )) {
                        const char* path = static_cast<const char*>( payload->Data );
                        if (const AssetEntry* e = assets.find( path ); e && e->kind == AssetKind::IMAGE) {
                            nc::RID new_tex = assets.ensure( path );
                            if (new_tex.is_valid()) {
                                nc::TypedVar tv( new_tex );
                                mat.set_param( nc::StringView( full ), tv );
                                field_changed = true;
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                if (ImGui::BeginPopupContextItem( "##texctx" )) {
                    if (ImGui::MenuItem( "Clear" )) {
                        mat.set_param( nc::StringView( full ), nc::TypedVar() );
                        field_changed = true;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            } else {
                ImGui::SetNextItemWidth( -FLT_MIN );
                field_changed |= draw_uniform_element( param, static_cast<uint8_t*>( edited.element_ptr( 0 ) ) );
            }
        }

        if (!param.is_texture() && field_changed) {
            mat.set_param( nc::StringView( full ), edited );
        }

        ImGui::PopID();
    }
}

void MaterialInspectorPlugin::parse_begin( EditorFrame& p_frame, const nc::types::TypeInfo* p_type, void* p_instance )
{
    ( void ) p_frame;
    ( void ) p_type;
    if (!p_instance)
        return;

    auto* mat = static_cast<nc::Material*>( p_instance );

    // Small preview control (Godot-style injection at the start of the material section).
    ImGui::SeparatorText( "Preview" );

    auto shader = mat->get_shader();
    ImGui::Text( "Shader: %s", shader ? "custom" : "default/none" );

    if (shader && !shader->filepath.empty()) {
        ImGui::SameLine();
        if (ImGui::SmallButton( "Reload" )) {
            assets.reload( shader->filepath );
            // The import driver + refresh_materials_using_shader will pick it up
        }
        ImGui::SetItemTooltip( "Reload shader and refresh materials using it" );
    }

    ImGui::SameLine();
    const ImVec4 preview_col = sample_material_preview_color( *mat );
    ImGui::ColorButton(
        "##matprev", preview_col, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2( 28, 28 )
    );
    ImGui::SetItemTooltip( "Live preview color sampled from common albedo/baseColor param (if present)" );
    ImGui::Spacing();
}

void MaterialInspectorPlugin::parse_end( EditorFrame& p_frame, const nc::types::TypeInfo* p_type, void* p_instance )
{
    ( void ) p_frame;
    ( void ) p_type;
    if (!p_instance)
        return;

    auto* mat = static_cast<nc::Material*>( p_instance );
    draw_material_params( assets, *mat );
}

} // namespace sea::editor
