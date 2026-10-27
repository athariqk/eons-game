#pragma once

#include <editor.h>

namespace nc::editor {

/**
 * @brief Property grid for the selected node.
 *
 * Draws the generic RTTI sheet; feature-specific sections come from
 * InspectorHandlers and value widgets from the property-widget registry
 * (both registered by plugins through EditorInterface).
 */
class InspectorPanel final : public EditorPanel {
public:
    InspectorPanel();

    void draw( EditorContext& ctx ) override;

    constexpr static float kLabelWidth = kInspectorLabelWidth;
    constexpr static float kIndentPx   = kInspectorIndentPx;

private:
    void draw_node( EditorContext& ctx, Node& selected );

    /** @brief Draw every reflected field of a component. */
    bool draw_property( const rtti::TypeInfo* type, void* data );

    char component_filter_[64] = {};

    /** @brief Context for the duration of draw_node (widget dispatch). */
    EditorContext* active_ctx_ = nullptr;
};

} // namespace nc::editor
