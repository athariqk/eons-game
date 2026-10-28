#include <algorithm>
#include <cstdio>

#include <ncore/core/rid.h>
#include <ncore/runtime/resources/resource_loader.h>
#include <ncore/services/video/render_service.h>

#include <panels/asset_picker.h>

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
    if (!picker.loader)
        return false;

    if (hint.gpu_texture) {
        if (!picker.thumbnails || !picker.renderer)
            return false;

        const RID texture =
            picker.thumbnails->texture_for( String( path.data(), path.size() ), *picker.loader, *picker.renderer );
        if (!texture)
            return false;

        value = texture;
        return true;
    }

    const RID rid = picker.loader->load( String( path.data(), path.size() ) );
    if (!rid)
        return false;

    value = rid;
    return true;
}

} // namespace

bool draw_asset_rid(
    const AssetPicker& picker, const rtti::RecordInfo* owner, StringView field, RID& value, bool& out_changed
)
{
    out_changed = false;

    const RidHint hint = rid_hint( owner, field );
    if (!hint.valid() || !picker.assets || !picker.loader)
        return false;

    if (!picker.assets->get_scanned())
        picker.assets->scan( *picker.loader, *picker.importer );

    // Paths are re-resolved on demand: assigning a value can insert into the
    // loader/cache maps and invalidate an earlier view.
    auto resolve_current = [&]() -> StringView {
        if (!value.is_valid())
            return {};
        return hint.gpu_texture ? picker.thumbnails->path_for_texture( value ) : picker.loader->path_of( value );
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
        for (const auto& entry : picker.assets->get_entries()) {
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
        if (hint.gpu_texture) {
            if (picker.renderer) {
                const RID fresh = picker.thumbnails->texture_for( path, *picker.loader, *picker.renderer, true );
                if (fresh) {
                    value       = fresh;
                    out_changed = true;
                }
            }
        } else {
            picker.loader->load( path, true );
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

void add_asset_property_widget( Editor& ed )
{
    ed.register_property_widget(
        rtti::TypeRegistry::find<RID>(),
        [assets = &ed.get_assets(), thumbnails = &ed.get_thumbnails(), loader = &ed.get_scene().get_resource_loader(),
         importer = &ed.get_importer()](
            EditorFrame& ctx, const rtti::RecordInfo* owner, StringView field, const rtti::TypeInfo* type, void* value,
            bool& out_changed
        ) {
            ( void ) type;
            auto renderer = ctx.scene->get_app_ctx()->services.resolve<RenderService>();
            AssetPicker picker{ assets, thumbnails, loader, importer, renderer };
            if (!picker.valid())
                return false;

            auto* rid = static_cast<RID*>( value );
            return draw_asset_rid( picker, owner, field, *rid, out_changed );
        }
    );
}

} // namespace nc::editor
