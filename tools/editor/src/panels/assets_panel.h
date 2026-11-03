#pragma once

#include <editor.h>

#include <io/editor_assets.h>

namespace nc {
class ResourceLoader;
} // namespace nc

namespace nc::editor {

/**
 * @brief Asset browser for asset previews, reload, and reimport.
 */
class AssetsPanel final : public EditorPanel {
public:
    AssetsPanel(
        AssetDatabase& p_assets NC_LIFETIME_BOUND, AssetImporter& p_importer NC_LIFETIME_BOUND,
        ThumbnailCache& p_thumbnails NC_LIFETIME_BOUND, ResourceLoader& p_loader NC_LIFETIME_BOUND,
        EditorSelection& p_selection NC_LIFETIME_BOUND
    );

    void draw_contents( EditorFrame& ctx ) override;

private:
    AssetDatabase& assets;
    AssetImporter& importer;
    ThumbnailCache& thumbnails;
    ResourceLoader& loader;
    EditorSelection& selection;

    char search_str[64]  = {};
    int kind_filter      = 0; // 0 = all kinds, otherwise AssetKind + 1
    bool stale_only      = false;
    int sort_column      = 0;
    bool sort_descending = false;
    bool grid_view       = true;
};

} // namespace nc::editor
