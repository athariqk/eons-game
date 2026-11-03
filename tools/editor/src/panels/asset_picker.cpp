#include <algorithm>
#include <cstdio>
#include <cstring>

#include <ncore/object/object_info.h>
#include <ncore/runtime/io/resource.h>
#include <ncore/runtime/io/resource_loader.h>
#include <ncore/runtime/scene.h>
#include <ncore/types/typedb.h>

#include <io/editor_assets.h>
#include <panels/asset_picker.h>

namespace nc::editor {

namespace {

/** Best-effort AssetKind filter from the static Ref target type name. */
AssetKind kind_hint_for_target( const types::RecordInfo* target )
{
    if (!target || !target->name)
        return AssetKind::UNKNOWN;

    const char* n = target->name;
    if (std::strncmp( n, "nc::", 4 ) == 0)
        n += 4;

    if (std::strcmp( n, "Image" ) == 0)
        return AssetKind::IMAGE;
    if (std::strcmp( n, "CubeMap" ) == 0)
        return AssetKind::CUBEMAP;
    if (std::strcmp( n, "Mesh" ) == 0)
        return AssetKind::MESH;
    if (std::strcmp( n, "MaterialShader" ) == 0)
        return AssetKind::MATERIAL_SHADER;
    if (std::strcmp( n, "Shader" ) == 0)
        return AssetKind::SHADER;
    if (std::strcmp( n, "AudioClip" ) == 0)
        return AssetKind::AUDIO;
    if (std::strcmp( n, "Font" ) == 0)
        return AssetKind::FONT;
    return AssetKind::UNKNOWN;
}

bool target_accepts_iresource( const types::RecordInfo* target )
{
    const types::RecordInfo* ir = TypeDB::find_record<IResource>();
    return ir && TypeDB::is_derived_or_same( target, ir );
}

const char* display_path_for( RefCounted* rc )
{
    if (!rc || !rc->is_a<IResource>())
        return nullptr;
    auto* res = static_cast<IResource*>( rc );
    if (res->filepath.empty())
        return nullptr;
    return res->filepath.c_str();
}

bool try_assign_path( ResourceLoader& loader, const types::IRefOps* ops, void* ref_slot, StringView path )
{
    if (!ops || path.empty())
        return false;

    const RID handle = loader.load( String( path ) );
    if (!handle.is_valid())
        return false;

    Ref<IResource> loaded = loader.get( handle );
    if (!loaded)
        return false;

    // share() acquires; returns false when the loaded resource is not a T.
    return ops->share( ref_slot, static_cast<RefCounted*>( loaded.get() ) );
}

bool draw_asset_popup(
    AssetDatabase& assets, ResourceLoader& loader, const types::IRefOps* ops, void* ref_slot,
    const types::RecordInfo* target, char* filter, size_t filter_cap
)
{
    bool changed         = false;
    const AssetKind hint = kind_hint_for_target( target );

    ImGui::InputTextWithHint( "##ref_filter", "Search...", filter, static_cast<int>( filter_cap ) );

    ImGui::BeginChild( "##ref_assets", ImVec2( 320.0f, 220.0f ), ImGuiChildFlags_Borders );

    for (const AssetEntry& entry : assets.get_entries()) {
        if (hint != AssetKind::UNKNOWN && entry.kind != hint)
            continue;
        if (filter[0] && !asset_path_matches( entry.path, filter ))
            continue;

        if (ImGui::Selectable( entry.path.c_str() )) {
            if (try_assign_path( loader, ops, ref_slot, entry.path )) {
                changed   = true;
                filter[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip( "%s (%s)", entry.path.c_str(), asset_kind_name( entry.kind ).data() );
        }
    }

    ImGui::EndChild();
    return changed;
}

} // namespace

bool draw_ref_property(
    EditorFrame& ctx, AssetDatabase& assets, ResourceLoader& loader, const types::TypeInfo* ref_type, void* ref_slot
)
{
    ( void ) ctx;

    if (!ref_type || !ref_slot)
        return false;

    const types::IRefOps* ops = ref_type->as_ref();
    if (!ops) {
        ImGui::TextDisabled( "Ref(ops missing)" );
        return false;
    }

    const types::RecordInfo* target = ops->target_type();
    RefCounted* rc                  = ops->get_refcounted( ref_slot );
    const types::RecordInfo* dyn    = ops->dynamic_type( ref_slot );
    const bool is_resource_slot     = target_accepts_iresource( target );

    bool changed = false;

    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    const float btn_w   = ImGui::CalcTextSize( "Assign" ).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float clear_w = ImGui::CalcTextSize( "x" ).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float total   = ImGui::CalcItemWidth();
    const float label_w = std::max( 40.0f, total - btn_w - clear_w - spacing * 2.0f );

    ImGui::SetNextItemWidth( label_w );

    if (!rc) {
        ImGui::TextDisabled( "None" );
    } else if (const char* path = display_path_for( rc )) {
        ImGui::TextUnformatted( path );
        if (ImGui::IsItemHovered()) {
            const char* tname = dyn && dyn->name ? dyn->name : ( target && target->name ? target->name : "IResource" );
            ImGui::SetTooltip( "%s\n%s", tname, path );
        }
    } else {
        const char* tname = dyn && dyn->name ? dyn->name : ( target && target->name ? target->name : "Ref" );
        ImGui::TextUnformatted( tname );
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip( "In-memory %s (no asset path)", tname );
    }

    // Accept ASSET_PATH drag-drop (payload set by Assets panel).
    if (is_resource_slot && ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "ASSET_PATH" )) {
            const char* path = static_cast<const char*>( payload->Data );
            if (path && try_assign_path( loader, ops, ref_slot, path ))
                changed = true;
        }
        ImGui::EndDragDropTarget();
    }

    if (is_resource_slot) {
        ImGui::SameLine( 0.0f, spacing );
        if (ImGui::SmallButton( "Assign" ))
            ImGui::OpenPopup( "##ref_assign" );

        if (ImGui::BeginPopup( "##ref_assign" )) {
            static char s_filter[64] = {};
            if (draw_asset_popup( assets, loader, ops, ref_slot, target, s_filter, sizeof( s_filter ) ))
                changed = true;
            ImGui::EndPopup();
        }
    } else {
        ImGui::SameLine( 0.0f, spacing );
        if (ImGui::SmallButton( "New" )) {
            if (ops->create( ref_slot, nullptr ))
                changed = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip( "Default-construct a new instance" );
    }

    ImGui::SameLine( 0.0f, spacing );
    ImGui::BeginDisabled( !rc );
    if (ImGui::SmallButton( "x" )) {
        ops->reset( ref_slot );
        changed = true;
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ))
        ImGui::SetTooltip( "Clear" );

    return changed;
}

} // namespace nc::editor
