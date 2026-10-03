#pragma once

// Inspector property grid — two-column ImGui table layout.
// Included from ncore_editor.cpp (implementation is header-only to keep the
// editor translation unit simple; no extra .cpp needed).

#include <imgui.h>

#include <ncore/core/types.h>

namespace nc::editor {
namespace propgrid {

using namespace nc::rtti;

// Fixed label column width in pixels. Stretch would fight nested TreeNodes;
// a fixed width keeps value widgets aligned across every component.
inline constexpr float kLabelWidth = 140.0f;
inline constexpr float kIndentPx   = 12.0f;

inline ImGuiDataType imgui_data_type( TypeKind kind )
{
    switch (kind) {
        case TypeKind::INT8:   return ImGuiDataType_S8;
        case TypeKind::UINT8:  return ImGuiDataType_U8;
        case TypeKind::INT16:  return ImGuiDataType_S16;
        case TypeKind::UINT16: return ImGuiDataType_U16;
        case TypeKind::INT32:  return ImGuiDataType_S32;
        case TypeKind::UINT32: return ImGuiDataType_U32;
        case TypeKind::INT64:  return ImGuiDataType_S64;
        case TypeKind::UINT64: return ImGuiDataType_U64;
        case TypeKind::FLOAT:  return ImGuiDataType_Float;
        case TypeKind::DOUBLE: return ImGuiDataType_Double;
        default:               return ImGuiDataType_COUNT;
    }
}

inline int string_resize_callback( ImGuiInputTextCallbackData* cb )
{
    if (cb->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* s = static_cast<String*>( cb->UserData );
        s->resize( static_cast<size_t>( cb->BufTextLen ) );
        cb->Buf = s->data();
    }
    return 0;
}

inline bool draw_leaf_value( const TypeInfo* type, void* ptr )
{
    switch (type->kind) {
        case TypeKind::BOOL:
            return ImGui::Checkbox( "##v", static_cast<bool*>( ptr ) );

        case TypeKind::INT8:
        case TypeKind::UINT8:
        case TypeKind::INT16:
        case TypeKind::UINT16:
        case TypeKind::INT32:
        case TypeKind::UINT32:
        case TypeKind::INT64:
        case TypeKind::UINT64:
            return ImGui::DragScalar( "##v", imgui_data_type( type->kind ), ptr, 1.0f );

        case TypeKind::FLOAT:
        case TypeKind::DOUBLE:
            return ImGui::DragScalar( "##v", imgui_data_type( type->kind ), ptr, 0.01f );

        case TypeKind::ENUM: {
            auto* info       = static_cast<const EnumInfo*>( type );
            int64_t current  = info->get_value( ptr );
            StringView cname = info->get_name( current );
            bool changed     = false;

            if (ImGui::BeginCombo( "##v", cname.data() )) {
                for (const auto& elem : info->elements()) {
                    bool selected = elem.value == current;
                    if (ImGui::Selectable( elem.name.data(), selected )) {
                        info->set_value( ptr, elem.value );
                        changed = true;
                    }
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            return changed;
        }

        case TypeKind::STRING: {
            auto* str = static_cast<String*>( ptr );
            return ImGui::InputText(
                "##v", str->data(), str->capacity() + 1, ImGuiInputTextFlags_CallbackResize, string_resize_callback,
                str
            );
        }

        default: {
            String out;
            type->to_string( out, ptr );
            ImGui::TextDisabled( "%s", out.c_str() );
            return false;
        }
    }
}

// Start a new property row. Label in column 0 (depth-indented); column 1 is
// ready for the value widget at full remaining width.
inline void begin_prop_row( StringView label, int depth )
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex( 0 );

    if (depth > 0) {
        ImGui::Dummy( ImVec2( kIndentPx * static_cast<float>( depth ), 0.0f ) );
        ImGui::SameLine( 0.0f, 0.0f );
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted( label.data(), label.data() + label.size() );

    ImGui::TableSetColumnIndex( 1 );
    ImGui::SetNextItemWidth( -FLT_MIN );
}

inline bool draw_record_fields( const RecordInfo* record, void* data, int depth );

inline bool draw_labeled_value( StringView label, const TypeInfo* type, void* ptr, int depth )
{
    if (!type) {
        begin_prop_row( label, depth );
        ImGui::TextDisabled( "<unregistered type>" );
        return false;
    }

    // Nested records: TreeNode in the label column; children continue as further
    // rows in the same table so columns stay aligned.
    if (type->kind == TypeKind::RECORD) {
        if (depth >= 8) {
            begin_prop_row( label, depth );
            ImGui::TextDisabled( "<max depth>" );
            return false;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );
        if (depth > 0) {
            ImGui::Dummy( ImVec2( kIndentPx * static_cast<float>( depth ), 0.0f ) );
            ImGui::SameLine( 0.0f, 0.0f );
        }

        String lbl( label );
        // NoTreePushOnOpen: avoid shifting the whole table; we indent via depth.
        constexpr ImGuiTreeNodeFlags kFlags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        bool open = ImGui::TreeNodeEx( lbl.c_str(), kFlags );

        ImGui::TableSetColumnIndex( 1 ); // empty value cell for the header row

        if (open) {
            // Do not TreePop when using NoTreePushOnOpen.
            return draw_record_fields( static_cast<const RecordInfo*>( type ), ptr, depth + 1 );
        }
        return false;
    }

    begin_prop_row( label, depth );
    return draw_leaf_value( type, ptr );
}

inline bool draw_field( const FieldInfo& field, void* data, int depth )
{
    if (field.is( PropertyFlags::HIDDEN ))
        return false;

    const TypeInfo* type = field.get_type();
    StringView name{ field.name.data(), field.name.size() };
    void* ptr = field.get_void_ptr( data );

    bool changed = false;

    ImGui::PushID( name.data(), name.data() + name.size() );
    ImGui::BeginDisabled( field.is( PropertyFlags::READ_ONLY ) );

    if (field.qualifier.is_pointer()) {
        begin_prop_row( name, depth );
        void* target = field.get_as<void*>( data );
        if (field.qualifier.is_cstring) {
            const char* s = static_cast<const char*>( target );
            ImGui::TextDisabled( "%s", s ? s : "(null)" );
        } else {
            ImGui::TextDisabled( "%p", target );
        }
    } else if (field.qualifier.is_array()) {
        std::string lbl = std::string( name ) + " [" + std::to_string( field.qualifier.array_length ) + "]";

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );
        if (depth > 0) {
            ImGui::Dummy( ImVec2( kIndentPx * static_cast<float>( depth ), 0.0f ) );
            ImGui::SameLine( 0.0f, 0.0f );
        }

        constexpr ImGuiTreeNodeFlags kFlags =
            ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        bool open = type && ImGui::TreeNodeEx( lbl.c_str(), kFlags );
        ImGui::TableSetColumnIndex( 1 );

        if (open && type) {
            auto* base = static_cast<uint8_t*>( ptr );
            for (uint32_t i = 0; i < field.qualifier.array_length; ++i) {
                ImGui::PushID( static_cast<int>( i ) );
                std::string idx = "[" + std::to_string( i ) + "]";
                changed |= draw_labeled_value( idx, type, base + i * type->size, depth + 1 );
                ImGui::PopID();
            }
        }
    } else {
        changed = draw_labeled_value( name, type, ptr, depth );
    }

    ImGui::EndDisabled();
    ImGui::PopID();
    return changed;
}

inline bool draw_record_fields( const RecordInfo* record, void* data, int depth )
{
    bool changed = false;
    for (const auto& field : record->fields())
        changed |= draw_field( field, data, depth );
    return changed;
}

// Entry point used by the Inspector. Opens a 2-column table and draws all fields.
inline bool draw_component_properties( const TypeInfo* type, void* data )
{
    NC_ASSERT( type->is_record() );

    auto record = static_cast<const RecordInfo*>( type );
    if (record->field_count() == 0)
        return false;

    // BordersInnerV = subtle vertical rule between label and value (toolbox feel).
    constexpr ImGuiTableFlags kFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX |
                                       ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoSavedSettings;

    if (!ImGui::BeginTable( "##propgrid", 2, kFlags ))
        return false;

    ImGui::TableSetupColumn( "Property", ImGuiTableColumnFlags_WidthFixed, kLabelWidth );
    ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_WidthStretch );

    bool changed = draw_record_fields( record, data, 0 );
    ImGui::EndTable();
    return changed;
}

} // namespace propgrid
} // namespace nc::editor
