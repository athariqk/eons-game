// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: inspector_panel.cpp

#include "inspector_panel.h"

#include <cstring>

#include <ncore/runtime/components.h>
#include <ncore/runtime/io/resource.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>

#include <editor/editor_components.h>
#include <editor/editor_selection.h>
#include <editor/fs/utils.h>
#include <editor/inspector/property_grid.h>
#include <editor/inspector/utils.h>
#include <editor/io/editor_assets.h>

namespace sea::editor {

namespace {

bool icontains( const char* hay, const char* needle )
{
    if (!needle || !*needle)
        return true;
    const size_t n = std::strlen( needle );
    for (; *hay; ++hay) {
        size_t i = 0;
        while (i < n && hay[i] &&
               std::tolower( static_cast<unsigned char>( hay[i] ) ) ==
                   std::tolower( static_cast<unsigned char>( needle[i] ) ))
            ++i;
        if (i == n)
            return true;
    }
    return false;
}

void format_bytes( uint64_t b, char* out, size_t n )
{
    if (b < 1024)
        std::snprintf( out, n, "%llu B", static_cast<unsigned long long>( b ) );
    else if (b < 1024ull * 1024)
        std::snprintf( out, n, "%.1f KB", static_cast<double>( b ) / 1024.0 );
    else
        std::snprintf( out, n, "%.2f MB", static_cast<double>( b ) / ( 1024.0 * 1024.0 ) );
}

void draw_badge( const char* text, ImU32 color )
{
    const ImVec2 ts = ImGui::CalcTextSize( text );
    const ImVec2 pad( 7.0f, 2.0f );
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 end( p.x + ts.x + pad.x * 2.0f, p.y + ts.y + pad.y * 2.0f );
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const ImU32 fill = ( color & 0x00FFFFFFu ) | ( 60u << 24 );
    dl->AddRectFilled( p, end, fill, 9.0f );
    dl->AddRect( p, end, color, 9.0f );
    dl->AddText( ImVec2( p.x + pad.x, p.y + pad.y ), color, text );
    ImGui::Dummy( ImVec2( end.x - p.x, end.y - p.y ) );
}

ImU32 status_color( AssetStatus s )
{
    switch (s) {
        case AssetStatus::READY:
            return IM_COL32( 110, 200, 120, 255 );
        case AssetStatus::DIRTY:
            return IM_COL32( 235, 200, 90, 255 );
        case AssetStatus::MISSING_SOURCE:
            return IM_COL32( 150, 150, 150, 255 );
        case AssetStatus::COMPILING:
            return IM_COL32( 100, 170, 240, 255 );
        default:
            return IM_COL32( 240, 100, 90, 255 );
    }
}

} // namespace

InspectorPanel::InspectorPanel(
    nc::Scene& p_scene, EditorSelection& p_selection, EditorAssets& p_assets, PropertyDrawerDB& p_drawers
) :
    EditorPanel( "Inspector", DockSlot::RIGHT_UPPER ), scene( p_scene ), selection( p_selection ), assets( p_assets ),
    property_drawers( p_drawers )
{}

void InspectorPanel::add_plugin( nc::Ptr<InspectorPlugin> p_plugin )
{
    if (p_plugin)
        plugins.push_back( std::move( p_plugin ) );
}

void InspectorPanel::draw_contents( EditorFrame& p_frame )
{
    if (selection.is_asset()) {
        draw_asset( p_frame, selection.get_selected_asset() );
        return;
    }

    nc::Node* selected = selection.node();
    if (!selected)
        ImGui::TextDisabled( "Nothing selected" );
    else
        draw_node( p_frame, *selected );
}

void InspectorPanel::draw_node( EditorFrame& p_frame, nc::Node& p_selected )
{
    auto& ecs = scene.get_ecs();

    draw_node_header( p_selected );
    ImGui::Spacing();
    draw_add_component( p_selected );
    ImGui::Spacing();

    const auto id = p_selected.get_id();

    for (auto& comp : p_selected.get_components()) {
        auto* type = ecs.resolve_component( comp.EcsId );
        if (!type || is_internal_component( type ))
            continue;

        void* data_ptr = p_selected.get_component( type );
        if (!data_ptr)
            continue;

        ImGui::PushID( reinterpret_cast<const void*>( static_cast<uintptr_t>( id ) ) );
        ImGui::PushID( reinterpret_cast<const void*>( static_cast<uintptr_t>( type->id.value ) ) );

        // ---- card header: title | [active] [...] ------------------------------
        char title[96];
        utils::pretty_name( type->name, title, sizeof title );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 8.0f, 6.0f ) );
        const bool open =
            ImGui::CollapsingHeader( title, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap );
        ImGui::PopStyleVar();
        ImGui::SetItemTooltip( "%s", type->name );
        ImGui::OpenPopupOnItemClick( "##compctx", ImGuiPopupFlags_MouseButtonRight );

        const ImVec2 hmin = ImGui::GetItemRectMin();
        const ImVec2 hmax = ImGui::GetItemRectMax();
        const float bh    = ImGui::GetFrameHeight();
        const float y     = hmin.y + ( ( hmax.y - hmin.y ) - bh ) * 0.5f;
        float x           = hmax.x - bh - 6.0f;

        ImGui::SetCursorScreenPos( ImVec2( x, y ) );
        if (ImGui::Button( "...##menu", ImVec2( bh, bh ) ))
            ImGui::OpenPopup( "##compctx" );

        x -= bh + 4.0f;
        ImGui::SetCursorScreenPos( ImVec2( x, y ) );
        ImGui::BeginDisabled( !comp.Toggleable );
        ImGui::Checkbox( "##active", &comp.Active );
        ImGui::EndDisabled();
        ImGui::SetItemTooltip( "Enabled" );

        ImGui::SetCursorScreenPos( ImVec2( hmin.x, hmax.y ) );
        ImGui::Dummy( ImVec2( 0.0f, 0.0f ) );

        if (ImGui::BeginPopup( "##compctx" )) {
            if (ImGui::MenuItem( "Reset" )) {
                nc::TypedVar def = nc::TypedVar::default_of( type );
                if (def) {
                    type->replace( def.element_ptr( 0 ), data_ptr );
                    p_selected.notify_changed( type );
                }
            }
            if (ImGui::MenuItem( "Copy Values" ))
                clipboard.set( nc::TypedPtr( type, data_ptr ) );
            const bool can_paste =
                clipboard.has_value() && clipboard.get_type() && clipboard.get_type()->id == type->id;
            if (ImGui::MenuItem( "Paste Values", nullptr, false, can_paste )) {
                type->replace( clipboard.element_ptr( 0 ), data_ptr );
                p_selected.notify_changed( type );
            }
            ImGui::Separator();
            if (ImGui::MenuItem( "Remove Component" ))
                to_remove.emplace( type );
            ImGui::EndPopup();
        }

        if (open) {
            if (draw_body( p_frame, type, data_ptr ))
                p_selected.notify_changed( type );
            ImGui::Spacing();
        }

        ImGui::PopID();
        ImGui::PopID();
    }

    for (auto* type : to_remove)
        p_selected.remove_component( type );
    to_remove.reset();
}

void InspectorPanel::draw_node_header( nc::Node& p_node )
{
    const auto name = p_node.get_name();
    const auto id   = p_node.get_id();
    auto active     = p_node.get_active();

    ImGui::Checkbox( "##node_active", active );
    ImGui::SetItemTooltip( "Active" );
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted( name.data() );

    char idbuf[32];
    std::snprintf( idbuf, sizeof idbuf, "ID %llu", static_cast<unsigned long long>( id ) );
    ImGui::SameLine();
    const float w     = ImGui::CalcTextSize( idbuf ).x;
    const float avail = ImGui::GetContentRegionAvail().x;
    if (avail > w)
        ImGui::SetCursorPosX( ImGui::GetCursorPosX() + avail - w );
    ImGui::TextDisabled( "%s", idbuf );

    ImGui::Separator();
}

void InspectorPanel::draw_add_component( nc::Node& p_node )
{
    auto& ecs = scene.get_ecs();

    if (ImGui::Button( "+  Add Component", ImVec2( -FLT_MIN, 0 ) ))
        ImGui::OpenPopup( "AddComponent" );

    const ImVec2 mn = ImGui::GetItemRectMin();
    const ImVec2 mx = ImGui::GetItemRectMax();
    ImGui::SetNextWindowPos( ImVec2( mn.x, mx.y + 2.0f ) );
    ImGui::SetNextWindowSizeConstraints( ImVec2( mx.x - mn.x, 0.0f ), ImVec2( mx.x - mn.x, 360.0f ) );

    if (!ImGui::BeginPopup( "AddComponent" ))
        return;

    if (ImGui::IsWindowAppearing()) {
        add_cursor = 0;
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::SetNextItemWidth( -FLT_MIN );
    if (ImGui::InputTextWithHint( "##filter", "Search components...", component_filter, sizeof( component_filter ) ))
        add_cursor = 0;

    nc::DynamicArray<const nc::types::TypeInfo*> items;
    for (auto type : ecs.get_component_types()) {
        if (!type->is_record() || is_internal_component( type ) || p_node.has_component( type ))
            continue;
        if (!icontains( type->name, component_filter ))
            continue;
        items.push_back( type );
    }
    std::sort( items.begin(), items.end(), []( auto* a, auto* b ) { return std::strcmp( a->name, b->name ) < 0; } );

    const int count = static_cast<int>( items.size() );
    bool moved      = false;
    if (ImGui::IsKeyPressed( ImGuiKey_DownArrow )) {
        ++add_cursor;
        moved = true;
    }
    if (ImGui::IsKeyPressed( ImGuiKey_UpArrow )) {
        --add_cursor;
        moved = true;
    }
    add_cursor = std::clamp( add_cursor, 0, std::max( 0, count - 1 ) );

    int pick = -1;
    if (count > 0 && ( ImGui::IsKeyPressed( ImGuiKey_Enter ) || ImGui::IsKeyPressed( ImGuiKey_KeypadEnter ) ))
        pick = add_cursor;

    if (ImGui::BeginChild( "##complist", ImVec2( 0, 220 ), ImGuiChildFlags_Borders )) {
        if (count == 0)
            ImGui::TextDisabled( "No matching components" );

        for (int i = 0; i < count; ++i) {
            char label[96];
            utils::pretty_name( items[static_cast<size_t>( i )]->name, label, sizeof label );
            ImGui::PushID( i );
            const bool sel = i == add_cursor;
            if (ImGui::Selectable( label, sel ))
                pick = i;
            if (ImGui::IsItemHovered() &&
                ( ImGui::GetIO().MouseDelta.x != 0.0f || ImGui::GetIO().MouseDelta.y != 0.0f ))
                add_cursor = i;
            if (sel && moved)
                ImGui::SetScrollHereY();
            ImGui::SetItemTooltip( "%s", items[static_cast<size_t>( i )]->name );
            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    if (pick >= 0) {
        p_node.add_component( items[static_cast<size_t>( pick )] );
        component_filter[0] = '\0';
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void InspectorPanel::draw_asset( EditorFrame& p_frame, nc::StringView p_path )
{
    const AssetEntry* entry = assets.find( p_path );
    if (!entry) {
        ImGui::TextDisabled( "Asset not found in the index" );
        if (ImGui::Button( "Deselect" ))
            selection.clear();
        return;
    }

    bool deselect = false;
    ImGui::PushID( entry->path.c_str() );

    // ---- Header: name, folder, badges -----------------------------------------
    const char* full  = entry->path.c_str();
    const char* slash = std::strrchr( full, '/' );
    const char* name  = slash ? slash + 1 : full;

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted( name );
    const float bw = ImGui::GetFrameHeight();
    ImGui::SameLine( ImGui::GetContentRegionMax().x - bw );
    if (ImGui::Button( "x##deselect", ImVec2( bw, bw ) ))
        deselect = true;
    ImGui::SetItemTooltip( "Deselect" );

    if (slash)
        ImGui::TextDisabled( "%.*s", static_cast<int>( slash - full ), full );

    const nc::StringView kind = asset_kind_name( entry->kind );
    char kind_buf[32];
    std::snprintf( kind_buf, sizeof kind_buf, "%.*s", static_cast<int>( kind.size() ), kind.data() );
    draw_badge( kind_buf, IM_COL32( 150, 170, 230, 255 ) );
    ImGui::SameLine();
    const nc::StringView status = asset_status_name( entry->status );
    char status_buf[32];
    std::snprintf( status_buf, sizeof status_buf, "%.*s", static_cast<int>( status.size() ), status.data() );
    draw_badge( status_buf, status_color( entry->status ) );

    if (entry->status == AssetStatus::ERROR && !entry->last_error.empty())
        ImGui::TextColored( ImVec4( 1.0f, 0.4f, 0.35f, 1.0f ), "%s", entry->last_error.c_str() );

    ImGui::Separator();

    // ---- Import (collapsed: properties are primary) --------------------------
    if (ImGui::CollapsingHeader( "Import" )) {
        if (ImGui::BeginTable( "##import", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg )) {
            ImGui::TableSetupColumn( "k", ImGuiTableColumnFlags_WidthFixed, 90.0f );
            ImGui::TableSetupColumn( "v", ImGuiTableColumnFlags_WidthStretch );

            auto row = [&]( const char* k ) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex( 0 );
                ImGui::TextDisabled( "%s", k );
                ImGui::TableSetColumnIndex( 1 );
            };

            char sz[32];
            format_bytes( entry->compiled_size, sz, sizeof sz );
            row( "Compiled" );
            ImGui::TextUnformatted( sz );

            format_bytes( entry->source_size, sz, sizeof sz );
            row( "Source size" );
            ImGui::TextUnformatted( sz );

            row( "Source" );
            ImGui::TextWrapped( "%s", entry->source_path.empty() ? "<not found>" : entry->source_path.c_str() );

            row( "Hash" );
            ImGui::Text( "%016llX", static_cast<unsigned long long>( entry->source_hash ) );

            ImGui::EndTable();
        }

        ImGui::Spacing();
        const float bwid = ( ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2.0f ) / 3.0f;

        const bool reimportable = asset_kind_can_import( entry->kind );
        ImGui::BeginDisabled( !assets.can_import() || !reimportable );
        if (ImGui::Button( "Reimport", ImVec2( bwid, 0 ) ))
            assets.reimport( entry->path );
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button( "Reload", ImVec2( bwid, 0 ) ))
            assets.reload( entry->path );

        ImGui::SameLine();
        ImGui::BeginDisabled( entry->source_path.empty() );
        if (ImGui::Button( "Reveal", ImVec2( bwid, 0 ) ))
            reveal_in_explorer( entry->source_path );
        ImGui::EndDisabled();

        AssetImportResult result;
        if (assets.last_result( entry->path, result ) && !result.ok)
            ImGui::TextColored( ImVec4( 1.0f, 0.35f, 0.3f, 1.0f ), "%s", result.message.c_str() );
    }

    // ---- Live IResource property sheet ---------------------------------------
    nc::Ref<nc::IResource> resource = assets.load<nc::IResource>( entry->path );

    if (!resource) {
        ImGui::Separator();
        ImGui::TextDisabled( "Resource not loaded" );
        if (ImGui::Button( "Load" ))
            assets.ensure( entry->path, true );
    } else {
        nc::IResource* res_ptr                  = resource.get();
        const nc::types::RecordInfo& class_info = res_ptr->get_class_info();

        char title[96];
        utils::pretty_name( class_info.name ? class_info.name : "Resource", title, sizeof title );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 8.0f, 6.0f ) );
        const bool open = ImGui::CollapsingHeader( title, ImGuiTreeNodeFlags_DefaultOpen );
        ImGui::PopStyleVar();
        if (open) {
            // Dynamic type, so Material / Mesh / Image fields show up, not only IResource.
            // Use draw_body so plugins (e.g. MaterialInspectorPlugin for shader uniforms) run.
            draw_body( p_frame, &class_info, res_ptr );
        }
    }

    ImGui::PopID();
    if (deselect)
        selection.clear();
}

bool InspectorPanel::draw_body( EditorFrame& frame, const nc::types::TypeInfo* p_type, void* p_data )
{
    constexpr int MAX_ACTIVE = 8;
    InspectorPlugin* active[MAX_ACTIVE];
    int n         = 0;
    bool replaced = false;

    for (auto& p : plugins) {
        if (n < MAX_ACTIVE && p->can_handle( p_type, p_data ))
            active[n++] = p.get();
    }

    for (int i = 0; i < n; ++i) {
        active[i]->parse_begin( frame, p_type, p_data );
        replaced |= active[i]->replaces_default();
    }

    bool changed = false;
    if (!replaced) {
        nc::Span<InspectorPlugin* const> active_plugins( active, static_cast<size_t>( n ) );
        PropertyGrid grid( &assets, &property_drawers, &frame, active_plugins );
        changed = grid.draw( nc::TypedPtr( p_type, p_data ) );
    }

    for (int i = n - 1; i >= 0; --i)
        active[i]->parse_end( frame, p_type, p_data );

    return changed;
}

} // namespace sea::editor
