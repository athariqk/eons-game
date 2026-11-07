// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: property_grid.h - InspectorPanel property sheet

#pragma once

#include <span>

#include <imgui.h>

#include <ncore/types/variant.h>

#include "inspector_plugin.h"

namespace nc::types {
struct IRefOps;
}

namespace sea::editor {

class EditorAssets;
class PropertyDrawerDB;
struct EditorFrame;

/**
 * @brief Two-column RTTI property sheet (label | value).
 *
 * Dispatch order per value: registered PropertyDrawerFn (exact type) -> primitives,
 * enums, strings -> Ref<T> picker -> vectors / fixed arrays -> compact row for
 * unregistered 2-4 component float/int32 records -> expandable record tree.
 */
class PropertyGrid {
public:
    explicit PropertyGrid(
        EditorAssets* assets NC_LIFETIME_BOUND, const PropertyDrawerDB* drawers NC_LIFETIME_BOUND = nullptr,
        EditorFrame* frame NC_LIFETIME_BOUND                               = nullptr,
        std::span<InspectorPlugin* const> active_plugins NC_LIFETIME_BOUND = {}
    );

    PropertyGrid( const PropertyGrid& )            = delete;
    PropertyGrid& operator=( const PropertyGrid& ) = delete;

    /**
     * @brief Draw every visible field of the record @p p_value points at.
     * @return true if any field was modified.
     */
    bool draw( const nc::TypedPtr& p_value );

    // Exposed so grid-integrated plugin drawers (e.g. material shader params)
    // can emit rows that participate in the current table + depth/indent.
    void begin_prop_row( const char* label, int depth );
    bool begin_tree_row( const char* label, int depth, ImGuiTreeNodeFlags flags = 0 );

    static constexpr float LABEL_WIDTH         = 150.0f;
    static constexpr float INDENT_PX           = 16.0f;
    static constexpr int MAX_DEPTH             = 8;
    static constexpr size_t MAX_ROWS           = 128; // elements drawn per vector/array
    static constexpr size_t MAX_PRIMITIVE_ROWS = 64;  // larger primitive vectors are summarized only

    /** @brief ImGui drag-drop payload carrying a NUL-terminated asset path. Match the asset browser. */
    static constexpr const char* ASSET_PAYLOAD = "NC_ASSET_PATH";

    enum ElemOp : int {
        OP_NONE = 0,
        OP_REMOVE,
        OP_DUPLICATE,
        OP_MOVE_UP,
        OP_MOVE_DOWN
    };

private:
    struct ElemMenu {
        size_t index    = 0;
        size_t count    = 0;
        int op          = OP_NONE;
        size_t op_index = 0;
    };

    void draw_guides( int depth );
    void row_context();

    bool draw_record_body( const nc::TypedPtr& record, int depth );
    bool draw_field(
        const nc::types::FieldInfo& field, const nc::types::RecordInfo* declaring, const nc::TypedPtr& owner, int depth
    );
    bool draw_value(
        const char* label, const nc::TypedPtr& value, int depth, const nc::types::FieldInfo* field = nullptr,
        const nc::types::RecordInfo* declaring = nullptr
    );
    bool draw_record( const char* label, const nc::TypedPtr& value, int depth );
    bool draw_vector( const char* label, const nc::TypedPtr& value, int depth );
    bool draw_array( const char* label, const nc::types::TypeInfo* elem, void* base, size_t count, int depth );
    bool draw_ref( const char* label, const nc::TypedPtr& slot, int depth );
    bool pick_ref_popup( const nc::types::IRefOps* ops, const nc::TypedPtr& slot, const char* current_path );
    bool assign_ref_path( const nc::types::IRefOps* ops, const nc::TypedPtr& slot, nc::StringView path );

    static bool is_object_derived( const nc::types::TypeInfo* type );
    static const nc::types::RecordInfo* resolve_record_info( const nc::types::TypeInfo* static_type, void* instance );

    EditorAssets* assets            = nullptr;
    const PropertyDrawerDB* drawers = nullptr;
    EditorFrame* frame              = nullptr;
    std::span<InspectorPlugin* const> inspector_plugins{};
    ElemMenu* menu = nullptr; // set by draw_vector for the next row only
};

} // namespace sea::editor
