#pragma once

#include <editor.h>

#include <io/editor_assets.h>
#include <io/thumbnail_cache.h>

namespace nc {
class ResourceLoader;
} // namespace nc

namespace nc::editor {

/** @brief Services the RID widget needs to resolve an asset handle. */
struct AssetPicker {
    AssetDatabase* assets      = nullptr;
    ThumbnailCache* thumbnails = nullptr;
    ResourceLoader* loader     = nullptr;
    AssetImporter* importer    = nullptr;
    RenderService* renderer    = nullptr;

    bool valid() const
    {
        return assets && thumbnails && loader && importer;
    }
};

struct RidHint {
    AssetKind kind   = AssetKind::UNKNOWN;
    bool gpu_texture = false;

    bool valid() const
    {
        return kind != AssetKind::UNKNOWN;
    }
};

/**
 * @brief Register the asset-backed RID property widget with the editor.
 */
void add_asset_property_widget( Editor& ed );

/**
 * @brief RID widget backed by the asset index (used by the inspector).
 *
 * Returns false for RIDs without an asset hint so the grid falls back to hex.
 */
bool draw_asset_rid(
    const AssetPicker& picker, const rtti::RecordInfo* owner, StringView field, RID& value, bool& out_changed
);

} // namespace nc::editor
