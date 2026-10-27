#pragma once

#include <editor.h>

#include <io/editor_assets.h>

namespace nc {
class Node;
class ResourceLoader;
class Scene;
} // namespace nc

namespace nc::editor {

/** @brief Services the RID widget needs to resolve an asset handle. */
struct AssetPicker {
    AssetIndex* Assets      = nullptr;
    ResourceLoader* Loader  = nullptr;
    RenderService* renderer = nullptr;

    bool valid() const
    {
        return Assets && Loader;
    }
};

struct RidHint {
    AssetKind kind  = AssetKind::UNKNOWN;
    bool GpuTexture = false;

    bool valid() const
    {
        return kind != AssetKind::UNKNOWN;
    }
};

/**
 * @brief Asset browser for asset previews, reload, and reimport.
 */
class AssetsPanel final : public EditorPanel {
public:
    AssetsPanel();

    void draw( EditorContext& ctx ) override;

private:
    char search_str[64] = {};
    int kind_filter = 0; // 0 = all kinds, otherwise AssetKind + 1
};

/**
 * @brief Register the asset-backed RID property widget with the editor.
 *
 * The widget resolves its asset index/loader/renderer from EditorContext, so it
 * only exists while the assets plugin is loaded.
 */
void add_asset_property_widget( EditorInterface& ed );

/**
 * @brief RID widget backed by the asset index (used by the inspector).
 *
 * Returns false for RIDs without an asset hint so the grid falls back to hex.
 */
bool draw_asset_rid(
    const AssetPicker& picker, const rtti::RecordInfo* owner, StringView field, RID& value, bool& out_changed
);

/**
 * @brief Create a node rendering a mesh asset (drag-drop from the asset list).
 *
 * Queued through Scene: entity creation cannot happen while the ECS runs.
 */
bool spawn_mesh_from_asset( Scene& scene, Node& parent, StringView path, ResourceLoader& loader );

} // namespace nc::editor
