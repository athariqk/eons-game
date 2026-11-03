#pragma once

#include <editor.h>

namespace nc::editor {

/**
 * @brief Property grid for the selected node.
 *
 * Draws the generic RTTI sheet; feature-specific sections come from
 * InspectorHandlers and value widgets from the property-widget registry.
 */
class InspectorPanel final : public EditorPanel {
public:
    InspectorPanel(
        Scene& scene NC_LIFETIME_BOUND, EditorSelection& selection NC_LIFETIME_BOUND,
        AssetDatabase& assets NC_LIFETIME_BOUND, AssetImporter& importer NC_LIFETIME_BOUND,
        ThumbnailCache& thumbnails NC_LIFETIME_BOUND, ResourceLoader& loader NC_LIFETIME_BOUND,
        const DynamicArray<Ptr<InspectorHandler>>& handlers NC_LIFETIME_BOUND,
        const DynamicArray<PropertyWidget>& widgets NC_LIFETIME_BOUND
    );

    void draw_contents( EditorFrame& ctx ) override;

    constexpr static float LABEL_WIDTH = INSPECTOR_LABEL_WIDTH;
    constexpr static float INDENT_PX   = INSPECTOR_INDENT_PX;

private:
    void draw_node( EditorFrame& ctx, Node& selected );

    /**
     * @brief Inspector body for a selected asset (browser selection).
     */
    void draw_asset( EditorFrame& ctx, StringView p_path );

    /**
     * @brief Draw every reflected field of a component.
     */
    bool draw_property( const types::TypeInfo* type, void* data );

    Scene& scene;
    EditorSelection& selection;
    AssetDatabase& assets;
    AssetImporter& importer;
    ThumbnailCache& thumbnails;
    ResourceLoader& loader;
    const DynamicArray<Ptr<InspectorHandler>>& handlers;
    const DynamicArray<PropertyWidget>& widgets;

    char component_filter[64] = {};

    /**
     * @brief Context for the duration of draw_node (widget dispatch).
     */
    EditorFrame* active_ctx = nullptr;

    BumpAllocator<const types::TypeInfo*> to_remove;
};

} // namespace nc::editor
