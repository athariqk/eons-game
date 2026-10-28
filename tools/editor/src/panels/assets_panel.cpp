#include <algorithm>
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

#include <fs/utils.h>
#include <io/asset_importer.h>
#include <io/editor_assets.h>
#include <io/thumbnail_cache.h>
#include <panels/assets_panel.h>

namespace nc::editor {

namespace {

StringView kind_label( int filter )
{
    if (filter == 0)
        return "All";
    return asset_kind_name( static_cast<AssetKind>( filter - 1 ) );
}

ImVec4 status_color( AssetStatus status )
{
    switch (status) {
        case AssetStatus::Dirty:
            return ImVec4( 1.0f, 0.72f, 0.25f, 1.0f ); // orange
        case AssetStatus::MissingSource:
            return ImVec4( 1.0f, 0.60f, 0.20f, 1.0f ); // darker orange
        case AssetStatus::Compiling:
            return ImVec4( 0.45f, 0.75f, 1.0f, 1.0f ); // blue
        case AssetStatus::Error:
            return ImVec4( 1.0f, 0.35f, 0.30f, 1.0f ); // red
        default:
            return ImVec4( 0.6f, 0.6f, 0.6f, 1.0f );
    }
}

const char* status_glyph( AssetStatus status )
{
    switch (status) {
        case AssetStatus::Dirty:
            return "*";
        case AssetStatus::MissingSource:
            return "?";
        case AssetStatus::Compiling:
            return "...";
        case AssetStatus::Error:
            return "!";
        default:
            return "";
    }
}

String leaf_name( StringView path )
{
    const size_t slash    = path.rfind( '/' );
    const StringView leaf = slash == StringView::npos ? path : path.substr( slash + 1 );
    return String( leaf.data(), leaf.size() );
}

void select_asset( EditorSelection& selection, EcsWorld& world, StringView path )
{
    selection.clear( world );
    selection.select_asset( path );
}

void asset_context_menu(
    AssetDatabase& index, ThumbnailCache& thumbnails, ResourceLoader& loader, RenderService& renderer,
    AssetImporter& importer, EditorSelection& selection, EcsWorld& world, const AssetEntry& entry
)
{
    if (!ImGui::BeginPopupContextItem( "##asset_ctx" ))
        return;

    const bool reimportable =
        entry.origin == AssetOrigin::File ? asset_kind_can_import( entry.kind ) : index.has_generator( entry.path );

    ImGui::BeginDisabled( !importer.ready() || !reimportable );
    if (ImGui::MenuItem( "Reimport" )) {
        if (entry.origin == AssetOrigin::Procedural)
            index.load_or_generate( entry.path, loader, renderer, true );
        else
            importer.enqueue( entry.path );
    }
    ImGui::EndDisabled();

    if (ImGui::MenuItem( "Reload" )) {
        if (entry.kind == AssetKind::IMAGE)
            thumbnails.texture_for( entry.path, loader, renderer, true );
        else
            loader.load( entry.path, true );
    }

    ImGui::Separator();

    if (ImGui::MenuItem( "Reveal in Explorer", nullptr, false, !entry.source_path.empty() ))
        reveal_in_explorer( entry.source_path );

    if (ImGui::MenuItem( "Copy path" ))
        ImGui::SetClipboardText( entry.path.c_str() );

    if (ImGui::MenuItem( "Deselect" ))
        selection.clear( world );

    ImGui::EndPopup();
}

void draw_asset_grid(
    AssetDatabase& index, ThumbnailCache& thumbnails, ResourceLoader& loader, RenderService& renderer,
    EditorSelection& selection, EcsWorld& world, AssetImporter& importer,
    const DynamicArray<const AssetEntry*>& visible, float tile = 88.0f
)
{
    if (visible.empty())
        return;

    const ImGuiStyle& style = ImGui::GetStyle();
    const float pad         = 4.0f;
    const float label_h     = ImGui::GetTextLineHeight();
    const ImVec2 cell( tile + pad * 2, tile + pad * 2 + label_h + 2.0f );
    const float spacing_x = style.ItemSpacing.x;
    const float row_h     = cell.y + style.ItemSpacing.y;

    const int columns =
        std::max( 1, static_cast<int>( ( ImGui::GetContentRegionAvail().x + spacing_x ) / ( cell.x + spacing_x ) ) );
    const int count = static_cast<int>( visible.size() );
    const int rows  = ( count + columns - 1 ) / columns;

    const auto& selected_path = selection.get_selected_asset();

    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImGuiListClipper clipper;
    clipper.Begin( rows, row_h );
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            for (int col = 0; col < columns; ++col) {
                const int i = row * columns + col;
                if (i >= count)
                    break;
                if (col > 0)
                    ImGui::SameLine( 0.0f, spacing_x );

                const AssetEntry& entry = *visible[i];
                const bool selected     = selected_path == entry.path;

                ImGui::PushID( entry.path.c_str() );

                // one invisible button owns click, drag, tooltip, context menu
                const ImVec2 p0 = ImGui::GetCursorScreenPos();
                const ImVec2 p1( p0.x + cell.x, p0.y + cell.y );
                ImGui::InvisibleButton( "##tile", cell );
                const bool hovered = ImGui::IsItemHovered();

                if (ImGui::IsItemClicked( ImGuiMouseButton_Left ) ||
                    ( ImGui::IsItemClicked( ImGuiMouseButton_Right ) && !selected ))
                    select_asset( selection, world, entry.path );

                asset_context_menu( index, thumbnails, loader, renderer, importer, selection, world, entry );

                if (ImGui::BeginDragDropSource( ImGuiDragDropFlags_None )) {
                    ImGui::SetDragDropPayload( "ASSET_PATH", entry.path.c_str(), entry.path.size() + 1 );
                    ImGui::TextUnformatted( entry.path.c_str() );
                    ImGui::EndDragDropSource();
                } else if (hovered && ImGui::GetDragDropPayload() == nullptr) {
                    ImGui::SetTooltip( "%s", entry.path.c_str() );
                }

                // background / selection
                if (selected || hovered) {
                    dl->AddRectFilled(
                        p0, p1, ImGui::GetColorU32( selected ? ImGuiCol_Header : ImGuiCol_HeaderHovered ), 4.0f
                    );
                }
                if (selected)
                    dl->AddRect( p0, p1, ImGui::GetColorU32( ImGuiCol_HeaderActive ), 4.0f );

                // thumbnail / placeholder
                const ImVec2 t0( p0.x + pad, p0.y + pad );
                const ImVec2 t1( t0.x + tile, t0.y + tile );
                dl->AddRectFilled( t0, t1, ImGui::GetColorU32( ImGuiCol_FrameBg ), 3.0f );

                bool drew_thumb = false;
                if (entry.kind == AssetKind::IMAGE) {
                    if (const RID thumb = thumbnails.texture_for( entry.path, loader, renderer ); thumb) {
                        // TODO: if the cache exposes texture size, letterbox here instead of stretching.
                        dl->AddImage( reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( thumb.value ) ), t0, t1 );
                        drew_thumb = true;
                    }
                }
                if (!drew_thumb) {
                    const StringView label =
                        entry.kind == AssetKind::IMAGE ? StringView( "image" ) : asset_kind_name( entry.kind );
                    const char* b   = label.data();
                    const char* e   = b + label.size();
                    const ImVec2 ts = ImGui::CalcTextSize( b, e );
                    dl->AddText(
                        ImVec2( t0.x + ( tile - ts.x ) * 0.5f, t0.y + ( tile - ts.y ) * 0.5f ),
                        ImGui::GetColorU32( ImGuiCol_TextDisabled ), b, e
                    );
                }

                // badges overlaid on the thumbnail (no layout shift in the name row)
                if (entry.status != AssetStatus::Ready) {
                    const char* g   = status_glyph( entry.status );
                    const ImVec2 gs = ImGui::CalcTextSize( g );
                    const ImVec2 bp( t0.x + 3.0f, t0.y + 2.0f );
                    dl->AddRectFilled(
                        ImVec2( bp.x - 2, bp.y - 1 ), ImVec2( bp.x + gs.x + 2, bp.y + gs.y + 1 ),
                        IM_COL32( 0, 0, 0, 170 ), 3.0f
                    );
                    dl->AddText( bp, ImGui::ColorConvertFloat4ToU32( status_color( entry.status ) ), g );
                }
                if (entry.origin == AssetOrigin::Procedural) {
                    const ImVec2 ps = ImGui::CalcTextSize( "P" );
                    const ImVec2 bp( t1.x - ps.x - 5.0f, t0.y + 2.0f );
                    dl->AddRectFilled(
                        ImVec2( bp.x - 2, bp.y - 1 ), ImVec2( bp.x + ps.x + 2, bp.y + ps.y + 1 ),
                        IM_COL32( 0, 0, 0, 170 ), 3.0f
                    );
                    dl->AddText( bp, IM_COL32( 140, 217, 255, 255 ), "P" );
                }

                // name, clipped to the tile, centered if it fits
                const char* path = entry.path.c_str();
                const char* leaf = std::strrchr( path, '/' );
                leaf             = leaf ? leaf + 1 : path;
                const ImVec2 ns  = ImGui::CalcTextSize( leaf );
                const float nx   = t0.x + std::max( 0.0f, ( tile - ns.x ) * 0.5f );
                const ImVec2 n0( t0.x, t1.y + 2.0f );
                const ImVec2 n1( t1.x, n0.y + label_h );
                dl->PushClipRect( n0, n1, true );
                dl->AddText( ImVec2( nx, n0.y ), ImGui::GetColorU32( ImGuiCol_Text ), leaf );
                dl->PopClipRect();

                ImGui::PopID();
            }
        }
    }
    clipper.End();
}

void draw_status_bar( const AssetDatabase& index )
{
    int dirty_count  = 0;
    int failed_count = 0;
    for (const auto& entry : index.get_entries()) {
        if (entry.status != AssetStatus::Ready)
            ++dirty_count;
        if (entry.status == AssetStatus::Error)
            ++failed_count;
    }

    ImGui::TextDisabled(
        "%d assets · %d dirty · %d failed", static_cast<int>( index.get_entries().size() ), dirty_count, failed_count
    );
}

} // namespace

AssetsPanel::AssetsPanel(
    AssetDatabase& p_assets, AssetImporter& p_importer, ThumbnailCache& p_thumbnails, ResourceLoader& p_loader,
    EditorSelection& p_selection
) :
    EditorPanel( "Assets", DockSlot::Bottom ), assets( p_assets ), importer( p_importer ), thumbnails( p_thumbnails ),
    loader( p_loader ), selection( p_selection )
{}

void AssetsPanel::draw_contents( EditorFrame& p_ctx )
{
    auto renderer = p_ctx.scene->get_app_ctx()->services.resolve<RenderService>();
    if (!renderer)
        return;

    const auto matches = [&]( const AssetEntry& entry ) {
        if (kind_filter != 0 && static_cast<int>( entry.kind ) + 1 != kind_filter)
            return false;
        if (!asset_path_matches( entry.path, search_str ))
            return false;
        if (stale_only && entry.status == AssetStatus::Ready)
            return false;
        return true;
    };

    thumbnails.begin_frame();

    if (!assets.get_scanned())
        assets.scan( loader, importer );

    if (ImGui::SmallButton( "Rescan" ))
        assets.scan( loader, importer );

    ImGui::SameLine();
    ImGui::BeginDisabled( !importer.ready() );
    if (ImGui::SmallButton( "Reimport dirty" )) {
        for (const auto& entry : assets.get_entries()) {
            if (entry.status != AssetStatus::Dirty && entry.status != AssetStatus::Error &&
                entry.status != AssetStatus::MissingSource)
                continue;

            if (entry.origin == AssetOrigin::Procedural)
                assets.load_or_generate( entry.path, loader, *renderer, true );
            else if (asset_kind_can_import( entry.kind ))
                importer.enqueue( entry.path );
        }
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ))
        ImGui::SetTooltip( "Reimport every asset whose source no longer matches its compiled .bin" );

    ImGui::SameLine();
    if (ImGui::SmallButton( "Reload visible" )) {
        for (const auto& entry : assets.get_entries()) {
            if (kind_filter != 0 && static_cast<int>( entry.kind ) + 1 != kind_filter)
                continue;
            if (!asset_path_matches( entry.path, search_str ))
                continue;

            if (entry.kind == AssetKind::IMAGE)
                thumbnails.texture_for( entry.path, loader, *renderer, true );
            else
                loader.load( entry.path, true );
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip( "Reload every asset in the current folder matching the filter" );

    if (importer.busy()) {
        ImGui::SameLine();
        ImGui::TextDisabled( "queued: %d", importer.get_pending() );
    }

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
    ImGui::SetNextItemWidth( 200.0f );
    ImGui::InputTextWithHint( "##assetsearch", "Search assets...", search_str, sizeof( search_str ) );

    ImGui::SameLine();
    ImGui::Checkbox( "Dirty only", &stale_only );
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip( "Show only assets whose source no longer matches the compiled .bin" );

    ImGui::SameLine();
    if (ImGui::SmallButton( grid_view ? "List" : "Grid" ))
        grid_view = !grid_view;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip( grid_view ? "Switch to list view" : "Switch to grid view" );

    ImGui::Separator();

    const float footer_h = ImGui::GetFrameHeightWithSpacing();

    ImGui::SameLine();
    ImGui::BeginChild( "##asset_list", ImVec2( 0.0f, -footer_h ), false );

    DynamicArray<const AssetEntry*> visible;
    for (const auto& candidate : assets.get_entries()) {
        if (!matches( candidate ))
            continue;
        visible.push_back( &candidate );
    }

    const auto sort_visible = [&]() {
        std::sort( visible.begin(), visible.end(), [&]( const AssetEntry* a, const AssetEntry* b ) {
            int order = 0;
            switch (sort_column) {
                case 1:
                    order = static_cast<int>( a->kind ) - static_cast<int>( b->kind );
                    break;
                case 2:
                    order = a->compiled_size < b->compiled_size ? -1 : ( a->compiled_size > b->compiled_size ? 1 : 0 );
                    break;
                default:
                    order = a->path.compare( b->path.c_str() );
                    break;
            }
            if (order == 0)
                order = a->path.compare( b->path.c_str() );
            return sort_descending ? order > 0 : order < 0;
        } );
    };

    if (grid_view) {
        sort_visible();
        draw_asset_grid( assets, thumbnails, loader, *renderer, selection, p_ctx.scene->get_ecs(), importer, visible );
        ImGui::EndChild();
        draw_status_bar( assets );
        return;
    }

    constexpr ImGuiTableFlags FLAGS = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                      ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable |
                                      ImGuiTableFlags_Sortable;

    if (ImGui::BeginTable( "##assets", 4, FLAGS )) {
        ImGui::TableSetupColumn( "Asset", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort );
        ImGui::TableSetupColumn( "Kind", ImGuiTableColumnFlags_WidthFixed, 70.0f );
        ImGui::TableSetupColumn( "Size", ImGuiTableColumnFlags_WidthFixed, 70.0f );
        ImGui::TableSetupScrollFreeze( 0, 1 );
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs(); specs && specs->SpecsCount > 0) {
            sort_column       = specs->Specs[0].ColumnIndex;
            sort_descending   = specs->Specs[0].SortDirection == ImGuiSortDirection_Descending;
            specs->SpecsDirty = false;
        }

        sort_visible();

        for (const AssetEntry* entry_ptr : visible) {
            const AssetEntry& entry = *entry_ptr;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex( 0 );

            if (entry.status != AssetStatus::Ready) {
                ImGui::TextColored( status_color( entry.status ), "%s", status_glyph( entry.status ) );
                if (ImGui::IsItemHovered()) {
                    if (entry.status == AssetStatus::Error && !entry.last_error.empty())
                        ImGui::SetTooltip(
                            "%s: %s", asset_status_name( entry.status ).data(), entry.last_error.c_str()
                        );
                    else
                        ImGui::SetTooltip( "%s", asset_status_name( entry.status ).data() );
                }
                ImGui::SameLine();
            }

            if (entry.origin == AssetOrigin::Procedural) {
                ImGui::TextColored( ImVec4( 0.55f, 0.85f, 1.0f, 1.0f ), "[P]" );
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip( "procedural asset" );
                ImGui::SameLine();
            }

            if (entry.kind == AssetKind::IMAGE) {
                const RID thumb = thumbnails.texture_for( entry.path, loader, *renderer );
                if (thumb) {
                    ImGui::Image(
                        reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( thumb.value ) ), ImVec2( 18, 18 )
                    );
                    ImGui::SameLine();
                }
            }

            const bool asset_selected = selection.get_selected_asset() == entry.path;
            ImGui::Selectable( entry.path.c_str(), asset_selected, ImGuiSelectableFlags_AllowDoubleClick );
            if (ImGui::IsItemClicked())
                select_asset( selection, p_ctx.scene->get_ecs(), entry.path );

            ImGui::PushID( entry.path.c_str() );
            asset_context_menu(
                assets, thumbnails, loader, *renderer, importer, selection, p_ctx.scene->get_ecs(), entry
            );
            ImGui::PopID();

            if (ImGui::BeginDragDropSource( ImGuiDragDropFlags_None )) {
                ImGui::SetDragDropPayload( "ASSET_PATH", entry.path.c_str(), entry.path.size() + 1 );
                ImGui::TextUnformatted( entry.path.c_str() );
                ImGui::EndDragDropSource();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip( "%s", entry.path.c_str() );

            AssetImportResult result;
            if (importer.last_result( entry.path, result ) && !result.ok) {
                ImGui::SameLine();
                ImGui::TextColored( ImVec4( 1.0f, 0.35f, 0.3f, 1.0f ), "!" );
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip( "%s", result.message.c_str() );
            }

            ImGui::TableSetColumnIndex( 1 );
            const StringView kind_name = asset_kind_name( entry.kind );
            ImGui::TextDisabled( "%.*s", static_cast<int>( kind_name.size() ), kind_name.data() );

            ImGui::TableSetColumnIndex( 2 );
            ImGui::TextDisabled( "%.1f KB", static_cast<double>( entry.compiled_size ) / 1024.0 );
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();

    draw_status_bar( assets );
}

} // namespace nc::editor
