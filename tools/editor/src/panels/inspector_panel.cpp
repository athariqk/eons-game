#include <algorithm>
#include <functional>
#include <utility>

#include <ncore/object/object_info.h>
#include <ncore/runtime/components.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/video/render_service.h>
#include <ncore/structs/color.h>
#include <ncore/structs/matrix.h>
#include <ncore/structs/quaternion.h>
#include <ncore/structs/rect.h>
#include <ncore/structs/rid.h>
#include <ncore/types/typedb.h>
#include <ncore/structs/vector.h>
#include <ncore/utils/math.h>

#include "../editor_components.h"

#include <fs/utils.h>
#include <io/asset_importer.h>
#include <io/editor_assets.h>
#include <io/thumbnail_cache.h>
#include <panels/asset_picker.h>
#include <panels/inspector_panel.h>

namespace nc::editor {

InspectorPanel::InspectorPanel(
    Scene& p_scene, EditorSelection& p_selection, AssetDatabase& p_assets, AssetImporter& p_importer,
    ThumbnailCache& p_thumbnails, ResourceLoader& p_loader, const DynamicArray<Ptr<InspectorHandler>>& p_handlers,
    const DynamicArray<PropertyWidget>& p_widgets
) :
    EditorPanel( "Inspector", DockSlot::RightUpper ), scene( p_scene ), selection( p_selection ), assets( p_assets ),
    importer( p_importer ), thumbnails( p_thumbnails ), loader( p_loader ), handlers( p_handlers ), widgets( p_widgets )
{}

void InspectorPanel::draw_node( EditorFrame& p_ctx, Node& p_selected )
{
    auto& ecs = scene.get_ecs();

    const auto name = p_selected.get_name();
    const auto id   = p_selected.get_id();
    auto active     = p_selected.get_active();

    ImGui::Text( "Node: %s", name.data() );
    ImGui::Text( "ID: %llu", id );
    ImGui::Checkbox( "Active", active );
    ImGui::Separator();

    if (ImGui::Button( "+ Add Component", ImVec2( -FLT_MIN, 0 ) ))
        ImGui::OpenPopup( "AddComponent" );

    if (ImGui::BeginPopup( "AddComponent" )) {
        ImGui::InputTextWithHint( "##filter", "Search...", component_filter, sizeof( component_filter ) );

        ImGui::BeginChild( "##complist", ImVec2( 0, 200 ), ImGuiChildFlags_Borders );
        for (auto type : ecs.get_component_types()) {
            if (!type->is_record() || is_internal_component( type ))
                continue;
            if (component_filter[0] && !std::strstr( type->name, component_filter ))
                continue;

            const bool already = p_selected.has_component( type );
            ImGui::BeginDisabled( already );
            if (ImGui::Selectable( type->name )) {
                p_selected.add_component( type );
                component_filter[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }

    active_ctx = &p_ctx;
    for (auto& comp : p_selected.get_components()) {
        auto* type = ecs.resolve_component( comp.EcsId );
        if (!type || is_internal_component( type ))
            continue;

        void* data_ptr = p_selected.get_component( type );
        if (!data_ptr)
            continue;

        ImGui::Separator();
        if (!ImGui::CollapsingHeader( type->name, ImGuiTreeNodeFlags_DefaultOpen ))
            continue;

        ImGui::PushID( static_cast<int>( id << 16 | type->id.value ) );

        // Inspector handlers may take over the sheet entirely (e.g. a future
        // sprite editor); otherwise the generic sheet draws first and handlers
        // append their own sections.
        bool replaces_default = false;
        for (const auto& handler : handlers) {
            if (handler && handler->can_handle( type, data_ptr ) && handler->replaces_default()) {
                replaces_default = true;
                break;
            }
        }

        if (!replaces_default && draw_property( type, data_ptr )) {
            p_selected.notify_changed( type );
        }

        for (const auto& handler : handlers) {
            if (handler)
                handler->draw( p_ctx, p_selected, type, data_ptr );
        }

        if (ImGui::SmallButton( "Remove" ))
            to_remove.emplace( type );

        ImGui::BeginDisabled( !comp.Toggleable );
        ImGui::Checkbox( "Active", &comp.Active );
        ImGui::EndDisabled();
        ImGui::PopID();
    }

    active_ctx = nullptr;

    bool any_removed = false;
    for (auto* type : to_remove) {
        any_removed |= p_selected.remove_component( type );
    }
    if (any_removed)
        to_remove.reset();
}

void InspectorPanel::draw_contents( EditorFrame& ctx )
{
    if (selection.is_asset()) {
        draw_asset( ctx, selection.get_selected_asset() );
        return;
    }

    Node* selected = selection.node( scene );

    if (!selected)
        ImGui::TextDisabled( "Nothing selected" );
    else
        draw_node( ctx, *selected );
}

void InspectorPanel::draw_asset( EditorFrame& ctx, StringView p_path )
{
    const AssetEntry* entry = assets.find( p_path );
    if (!entry) {
        ImGui::TextDisabled( "Asset not found in the index" );
        if (ImGui::Button( "Deselect" ))
            selection.clear( ctx.scene->get_ecs() );
        return;
    }

    ImGui::TextUnformatted( entry->path.c_str() );
    ImGui::SameLine();
    ImGui::TextDisabled( "(%s)", asset_kind_name( entry->kind ).data() );
    ImGui::Separator();

    ImGui::Text( "Status: %s", asset_status_name( entry->status ).data() );
    ImGui::Text( "Size: %.1f KB", static_cast<double>( entry->compiled_size ) / 1024.0 );
    ImGui::TextWrapped( "Source: %s", entry->source_path.empty() ? "<not found>" : entry->source_path.c_str() );
    ImGui::Text( "Hash: %016llX", static_cast<unsigned long long>( entry->source_hash ) );
    ImGui::Separator();

    const bool reimportable = asset_kind_can_import( entry->kind );
    ImGui::BeginDisabled( !importer.ready() || !reimportable );
    if (ImGui::Button( "Reimport" ))
        importer.enqueue( entry->path );
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button( "Reload" )) {
        if (entry->kind == AssetKind::IMAGE)
            thumbnails.texture_for( entry->path, *ctx.scene, true );
        else
            loader.load( entry->path, true );
    }

    {
        AssetImportResult result;
        if (importer.last_result( entry->path, result ) && !result.ok)
            ImGui::TextColored( ImVec4( 1.0f, 0.35f, 0.3f, 1.0f ), "%s", result.message.c_str() );
    }

    ImGui::BeginDisabled( entry->source_path.empty() );
    if (ImGui::Button( "Reveal" ))
        reveal_in_explorer( entry->source_path );
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Separator();
    if (ImGui::Button( "Deselect" ))
        selection.clear( ctx.scene->get_ecs() );
}

/**
 * @brief Widget registry wrapper threaded through the property recursion.
 *
 * Widgets are matched by reflected
 * type; the first match owns the row.
 */
struct PropertyGridWidgets {
    const DynamicArray<PropertyWidget>* List = nullptr;
    EditorFrame* ctx                         = nullptr;
    AssetDatabase* assets                    = nullptr;
    ResourceLoader* loader                   = nullptr;

    bool handles( const types::TypeInfo* type ) const
    {
        if (!List)
            return false;
        for (const auto& widget : *List) {
            if (widget.Type == type)
                return true;
        }
        return false;
    }

    /** @brief Returns true when a widget owned the value; out_changed reports edits. */
    bool draw(
        const types::TypeInfo* type, const types::RecordInfo* owner, StringView field, void* value, bool& out_changed
    ) const
    {
        out_changed = false;
        if (!List || !ctx)
            return false;

        for (const auto& widget : *List) {
            if (widget.Type != type || !widget.Draw)
                continue;
            return widget.Draw( *ctx, owner, field, type, value, out_changed );
        }
        return false;
    }

    bool draw_ref( const types::TypeInfo* type, void* value ) const
    {
        if (!ctx || !assets || !loader || !type || !value)
            return false;
        return draw_ref_property( *ctx, *assets, *loader, type, value );
    }
};
