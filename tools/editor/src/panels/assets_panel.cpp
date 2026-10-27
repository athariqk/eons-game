#include <filesystem>
#include <string>

#include <ncore/core/vector.h>
#include <ncore/resources/resource.h>
#include <ncore/runtime/components/material.h>
#include <ncore/runtime/components/mesh.h>
#include <ncore/runtime/components/resource.h>
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/resources/resource_loader.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/video/render_service.h>

#include <io/asset_importer.h>
#include <io/editor_assets.h>
#include <panels/assets_panel.h>

namespace nc::editor {

namespace {

RidHint rid_hint( const rtti::RecordInfo* owner, StringView field )
{
    if (!owner || !owner->name)
        return {};

    auto is = [&]( std::string_view owner_name, std::string_view field_name ) {
        return owner_name == owner->name && field_name == field;
    };

    if (is( "nc::MeshComponent", "Source" ))
        return { AssetKind::MESH, false };
    if (is( "nc::MaterialComponent", "Shader" ))
        return { AssetKind::MATERIAL_SHADER, false };
    if (is( "nc::MaterialComponent", "Textures" ))
        return { AssetKind::IMAGE, true };
    if (is( "nc::SpriteComponent", "source" ))
        return { AssetKind::MESH, false };
    if (is( "nc::SpriteComponent", "texture" ))
        return { AssetKind::IMAGE, true };
    return {};
}

bool assign_from_path( const AssetPicker& picker, const RidHint& hint, StringView path, RID& value )
{
    if (path.empty()) {
        value = 0;
        return true;
    }
    if (!picker.Loader)
        return false;

    if (hint.GpuTexture) {
        if (!picker.Assets || !picker.renderer)
            return false;

        const RID texture =
            picker.Assets->texture_for( String( path.data(), path.size() ), *picker.Loader, *picker.renderer );
        if (!texture)
            return false;

        value = texture;
        return true;
    }

    const RID rid = picker.Loader->load( String( path.data(), path.size() ) );
    if (!rid)
        return false;

    value = rid;
    return true;
}

} // namespace

/**
 * @brief Combo + reload + drag-drop widget for asset-backed RIDs.
 *
 * Returns false for RIDs without an asset hint so the property grid falls back
 * to the raw hex editor.
 */
bool draw_asset_rid(
    const AssetPicker& picker, const rtti::RecordInfo* owner, StringView field, RID& value, bool& out_changed
)
{
    out_changed = false;

    const RidHint hint = rid_hint( owner, field );
    if (!hint.valid() || !picker.Assets || !picker.Loader)
        return false;

    if (!picker.Assets->scanned())
        picker.Assets->scan( picker.Loader->get_asset_root() );

    // Paths are re-resolved on demand: assigning a value can insert into the
    // loader/cache maps and invalidate an earlier view.
    auto resolve_current = [&]() -> StringView {
        if (!value.is_valid())
            return {};
        return hint.GpuTexture ? picker.Assets->path_for_texture( value ) : picker.Loader->path_of( value );
    };

    StringView current = resolve_current();

    char preview[192];
    if (!current.empty()) {
        std::snprintf( preview, sizeof( preview ), "%.*s", static_cast<int>( current.size() ), current.data() );
    } else if (value.is_valid()) {
        std::snprintf( preview, sizeof( preview ), "<RID 0x%llX>", static_cast<unsigned long long>( value.value ) );
    } else {
        std::snprintf( preview, sizeof( preview ), "<none>" );
    }

    ImGui::SetNextItemWidth( std::max( 40.0f, ImGui::CalcItemWidth() - 26.0f ) );
    if (ImGui::BeginCombo( "##asset", preview )) {
        static char filter[64] = {};
        ImGui::SetNextItemWidth( -FLT_MIN );
        ImGui::InputTextWithHint( "##asset_filter", "Search...", filter, sizeof( filter ) );

        if (ImGui::Selectable( "<none>", !value.is_valid() )) {
            value       = 0;
            out_changed = true;
        }

        ImGui::BeginChild( "##asset_list", ImVec2( 320.0f, 240.0f ), ImGuiChildFlags_Borders );
        for (const auto& entry : picker.Assets->entries()) {
            if (entry.kind != hint.kind || !asset_path_matches( entry.path, filter ))
                continue;

            if (ImGui::Selectable( entry.path.c_str(), StringView( entry.path ) == resolve_current() ))
                out_changed = assign_from_path( picker, hint, StringView( entry.path ), value );
        }
        ImGui::EndChild();
        ImGui::EndCombo();
    }

    current = resolve_current();

    ImGui::SameLine();
    ImGui::BeginDisabled( current.empty() );
    if (ImGui::SmallButton( "R" )) {
        const String path( current.data(), current.size() );
        if (hint.GpuTexture) {
            if (picker.renderer) {
                const RID fresh = picker.Assets->texture_for( path, *picker.Loader, *picker.renderer, true );
                if (fresh) {
                    value       = fresh;
                    out_changed = true;
                }
            }
        } else {
            picker.Loader->load( path, true );
        }
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ))
        ImGui::SetTooltip( "Reload asset from disk" );

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "ASSET_PATH" ))
            out_changed = assign_from_path( picker, hint, static_cast<const char*>( payload->Data ), value );
        ImGui::EndDragDropTarget();
    }

    return true;
}

namespace {

StringView kind_label( int filter )
{
    if (filter == 0)
        return "All";
    return asset_kind_name( static_cast<AssetKind>( filter - 1 ) );
}

} // namespace

AssetsPanel::AssetsPanel() : EditorPanel( "Assets", DockSlot::Bottom ) {}

void AssetsPanel::draw( EditorContext& ctx )
{
    if (!ImGui::Begin( title.c_str(), &is_visible )) {
        ImGui::End();
        return;
    }

    if (!ctx.assets || !ctx.loader || !ctx.renderer || !ctx.importer) {
        ImGui::TextDisabled( "Asset services unavailable" );
        ImGui::End();
        return;
    }

    auto& index      = *ctx.assets;
    auto& importer   = *ctx.importer;
    auto& loader     = *ctx.loader;
    auto& renderer   = *ctx.renderer;
    const char* root = loader.get_asset_root();

    if (!index.scanned())
        index.scan( root );

    if (ImGui::SmallButton( "Rescan" ))
        index.scan( root );

    ImGui::SameLine();
    if (ImGui::SmallButton( "Reload changed" )) {
        for (const auto& entry : index.entries()) {
            const auto file = std::filesystem::path( root ) / ( entry.path + ".bin" );
            std::error_code ec;
            const auto mtime = std::filesystem::last_write_time( file, ec );
            if (ec)
                continue;
            if (mtime.time_since_epoch().count() != entry.Mtime) {
                if (entry.kind == AssetKind::IMAGE)
                    index.texture_for( entry.path, loader, renderer, true );
                else
                    loader.load( entry.path, true );
            }
        }
        index.scan( root );
    }

    ImGui::SameLine();
    ImGui::BeginDisabled( !importer.ready() );
    if (ImGui::SmallButton( "Reimport changed" )) {
        for (const auto& entry : index.entries()) {
            if (!asset_kind_can_import( entry.kind ) || !importer.source_newer( entry.path ))
                continue;

            importer.import( entry.path );
            if (entry.kind == AssetKind::IMAGE)
                index.texture_for( entry.path, loader, renderer, true );
        }
        index.scan( root );
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ))
        ImGui::SetTooltip( "Recompile every asset whose source is newer than its .bin" );

    ImGui::SameLine();
    ImGui::SetNextItemWidth( 110.0f );
    if (ImGui::BeginCombo( "##kindfilter", kind_label( kind_filter ).data() )) {
        for (int i = 0; i <= static_cast<int>( AssetKind::COUNT ); ++i) {
            if (ImGui::Selectable( kind_label( i ).data(), kind_filter == i ))
                kind_filter = i;
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth( -FLT_MIN );
    ImGui::InputTextWithHint( "##assetsearch", "Search assets...", search_str, sizeof( search_str ) );

    constexpr ImGuiTableFlags FLAGS = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                      ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;

    if (ImGui::BeginTable( "##assets", 4, FLAGS )) {
        ImGui::TableSetupColumn( "Asset", ImGuiTableColumnFlags_WidthStretch );
        ImGui::TableSetupColumn( "Kind", ImGuiTableColumnFlags_WidthFixed, 70.0f );
        ImGui::TableSetupColumn( "Size", ImGuiTableColumnFlags_WidthFixed, 70.0f );
        ImGui::TableSetupColumn( "##actions", ImGuiTableColumnFlags_WidthFixed, 62.0f );
        ImGui::TableSetupScrollFreeze( 0, 1 );
        ImGui::TableHeadersRow();

        for (const auto& entry : index.entries()) {
            if (kind_filter != 0 && static_cast<int>( entry.kind ) + 1 != kind_filter)
                continue;
            if (!asset_path_matches( entry.path, search_str ))
                continue;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex( 0 );

            if (importer.source_newer( entry.path )) {
                ImGui::TextColored( ImVec4( 1.0f, 0.72f, 0.25f, 1.0f ), "*" );
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip( "Source is newer than the compiled asset" );
                ImGui::SameLine();
            }

            if (entry.kind == AssetKind::IMAGE) {
                const RID thumb = index.texture_for( entry.path, loader, renderer );
                if (thumb) {
                    ImGui::Image(
                        reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( thumb.value ) ), ImVec2( 18, 18 )
                    );
                    ImGui::SameLine();
                }
            }

            ImGui::Selectable( entry.path.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick );
            if (ImGui::BeginDragDropSource( ImGuiDragDropFlags_None )) {
                ImGui::SetDragDropPayload( "ASSET_PATH", entry.path.c_str(), entry.path.size() + 1 );
                ImGui::TextUnformatted( entry.path.c_str() );
                ImGui::EndDragDropSource();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip( "%s", entry.path.c_str() );

            if (const AssetImportResult* result = importer.last_result( entry.path ); result && !result->Ok) {
                ImGui::SameLine();
                ImGui::TextColored( ImVec4( 1.0f, 0.35f, 0.3f, 1.0f ), "!" );
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip( "%s", result->Message.c_str() );
            }

            ImGui::TableSetColumnIndex( 1 );
            const StringView kind_name = asset_kind_name( entry.kind );
            ImGui::TextDisabled( "%.*s", static_cast<int>( kind_name.size() ), kind_name.data() );

            ImGui::TableSetColumnIndex( 2 );
            ImGui::TextDisabled( "%.1f KB", static_cast<double>( entry.SizeBytes ) / 1024.0 );

            ImGui::TableSetColumnIndex( 3 );
            ImGui::PushID( entry.path.c_str() );

            if (ImGui::SmallButton( "R" )) {
                if (entry.kind == AssetKind::IMAGE)
                    index.texture_for( entry.path, loader, renderer, true );
                else
                    loader.load( entry.path, true );
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip( "Reload compiled asset" );

            ImGui::SameLine();
            ImGui::BeginDisabled( !importer.ready() || !asset_kind_can_import( entry.kind ) );
            if (ImGui::SmallButton( "I" )) {
                importer.import( entry.path );
                if (entry.kind == AssetKind::IMAGE)
                    index.texture_for( entry.path, loader, renderer, true );
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ))
                ImGui::SetTooltip( "Reimport: recompile from source, then reload" );

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void add_asset_property_widget( EditorInterface& ed )
{
    ed.add_property_widget(
        rtti::TypeRegistry::find<RID>(), []( EditorContext& ctx, const rtti::RecordInfo* owner, StringView field,
                                             const rtti::TypeInfo* type, void* value, bool& out_changed ) {
            ( void ) type;
            AssetPicker picker{ ctx.assets, ctx.loader, ctx.renderer };
            if (!picker.valid())
                return false;

            auto* rid = static_cast<RID*>( value );
            return draw_asset_rid( picker, owner, field, *rid, out_changed );
        }
    );
}

bool spawn_mesh_from_asset( Scene& scene, Node& parent, StringView path, ResourceLoader& loader )
{
    const EcsEntity parent_id = parent.get_id();
    const String asset_path( path.data(), path.size() );

    // Entity creation is queued: the editor runs inside an ECS system, where the
    // world is read-only and new entities would get deferred (temporary) ids.
    scene.queue_task( [&scene, parent_id, asset_path, &loader]() {
        auto* parent_node = scene.node_from_entity( parent_id );
        if (!parent_node)
            return;

        const RID mesh = loader.load( asset_path );
        if (!mesh)
            return;

        const std::string stem = std::filesystem::path( asset_path.c_str() ).stem().string();

        auto node = parent_node->create_child( String( stem.c_str() ) );
        node->add_component<Transform3DComponent>();
        node->add_component<HasResourceTag>();
        node->add_component<MeshComponent>( MeshComponent{ .source = mesh } );

        MaterialComponent material;
        material.shader = loader.load( "shaders/materials/standard_spatial.slang" );
        material.set_params<Vec4f>( "albedoColor", Vec4f( 1, 1, 1, 1 ) );
        node->add_component<MaterialComponent>( material );

        node->add_component<MeshRenderComponent>();
        node->add_component<MaterialRenderComponent>();
    } );

    return true;
}
} // namespace nc::editor
