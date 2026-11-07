// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: property_grid.cpp

#include "property_grid.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstdio>
#include <cstring>

#include <ncore/object/object_info.h>
#include <ncore/runtime/io/resource.h>
#include <ncore/runtime/resources/material.h>
#include <ncore/types/typedb.h>

#include "plugins/material_inspector.h" // for draw_material_params when we hit a Material record
#include "property_draw_context.h"
#include "property_drawer_db.h"
#include "utils.h"
#include "widgets/vector_widgets.h"

#include <editor/io/editor_assets.h>

namespace sea::editor {

namespace {

ImGuiDataType imgui_data_type( nc::types::TypeKind kind )
{
    switch (kind) {
        case nc::types::TypeKind::INT8:
            return ImGuiDataType_S8;
        case nc::types::TypeKind::UINT8:
            return ImGuiDataType_U8;
        case nc::types::TypeKind::INT16:
            return ImGuiDataType_S16;
        case nc::types::TypeKind::UINT16:
            return ImGuiDataType_U16;
        case nc::types::TypeKind::INT32:
            return ImGuiDataType_S32;
        case nc::types::TypeKind::UINT32:
            return ImGuiDataType_U32;
        case nc::types::TypeKind::INT64:
            return ImGuiDataType_S64;
        case nc::types::TypeKind::UINT64:
            return ImGuiDataType_U64;
        case nc::types::TypeKind::FLOAT:
            return ImGuiDataType_Float;
        case nc::types::TypeKind::DOUBLE:
            return ImGuiDataType_Double;
        default:
            return ImGuiDataType_COUNT;
    }
}

int string_resize_callback( ImGuiInputTextCallbackData* cb )
{
    if (cb->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* s = static_cast<nc::String*>( cb->UserData );
        s->resize( static_cast<size_t>( cb->BufTextLen ) );
        cb->Buf = s->data();
    }
    return 0;
}

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

void to_cstr( nc::StringView sv, char* out, size_t cap )
{
    const size_t n = std::min( sv.size(), cap - 1 );
    std::memcpy( out, sv.data(), n );
    out[n] = '\0';
}

const char* file_part( const char* path )
{
    const char* s = std::strrchr( path, '/' );
    return s ? s + 1 : path;
}

//------------------------------------------------------------------------------
// Compact fallback for unregistered Vec2/Vec4-like records
//------------------------------------------------------------------------------

struct InlineLayout {
    const nc::types::FieldInfo* f[4] = {};
    int n                            = 0;
    nc::types::TypeKind kind         = nc::types::TypeKind::INVALID;
};

/** 2-4 contiguous float or int32 fields, no parent class. */
bool inline_layout( const nc::types::RecordInfo* rec, InlineLayout& L )
{
    if (rec->kind != nc::types::TypeKind::RECORD || rec->parent_id.valid())
        return false;

    int n = 0;
    for (const auto& f : rec->fields()) {
        if (n >= 4)
            return false;
        if (f.is( nc::types::PropertyFlags::HIDDEN ) || f.qualifier.is_array() || f.qualifier.is_pointer())
            return false;
        const nc::types::TypeInfo* t = f.get_type();
        if (!t || ( t->kind != nc::types::TypeKind::FLOAT && t->kind != nc::types::TypeKind::INT32 ))
            return false;
        if (n == 0)
            L.kind = t->kind;
        else if (t->kind != L.kind || f.offset != L.f[0]->offset + static_cast<size_t>( n ) * 4)
            return false;
        L.f[n++] = &f;
    }
    L.n = n;
    return n >= 2;
}

bool draw_compact( const InlineLayout& L, void* inst )
{
    static const char* const FF[4] = { "X:%.3f", "Y:%.3f", "Z:%.3f", "W:%.3f" };
    static const char* const FI[4] = { "X:%d", "Y:%d", "Z:%d", "W:%d" };

    void* base = L.f[0]->get_void_ptr( inst );
    if (L.kind == nc::types::TypeKind::FLOAT)
        return widgets::drag_float_components(
            static_cast<float*>( base ), widgets::DragComponentsDesc{ .count = L.n, .speed = 0.01f, .formats = FF }
        );
    return widgets::drag_int_components(
        static_cast<int*>( base ), widgets::DragComponentsDesc{ .count = L.n, .speed = 1.0f, .formats = FI }
    );
}

bool apply_element_op(
    const nc::types::RecordInfo* rec, void* inst, const nc::types::TypeInfo* elem, size_t n, int op, size_t i
)
{
    auto at = [&]( size_t k ) -> void* {
        return static_cast<std::byte*>( rec->container_data( inst ) ) + k * elem->size;
    };

    switch (op) {
        case PropertyGrid::OP_REMOVE:
            for (size_t j = i; j + 1 < n; ++j)
                elem->replace( at( j + 1 ), at( j ) );
            rec->resize( inst, n - 1 );
            return true;
        case PropertyGrid::OP_DUPLICATE:
            rec->resize( inst, n + 1 );
            for (size_t j = n; j > i + 1; --j)
                elem->replace( at( j - 1 ), at( j ) );
            elem->replace( at( i ), at( i + 1 ) );
            return true;
        case PropertyGrid::OP_MOVE_UP:
        case PropertyGrid::OP_MOVE_DOWN: {
            const size_t a = op == PropertyGrid::OP_MOVE_UP ? i - 1 : i;
            nc::TypedVar tmp{ nc::TypedPtr( elem, at( a ) ) };
            elem->replace( at( a + 1 ), at( a ) );
            elem->replace( tmp.element_ptr( 0 ), at( a + 1 ) );
            return true;
        }
        default:
            return false;
    }
}

} // namespace

//------------------------------------------------------------------------------
// Basics
//------------------------------------------------------------------------------

PropertyGrid::PropertyGrid(
    EditorAssets* p_assets, const PropertyDrawerDB* p_drawers, EditorFrame* p_frame,
    std::span<InspectorPlugin* const> active_plugins
) : assets( p_assets ), drawers( p_drawers ), frame( p_frame ), inspector_plugins( active_plugins )
{}

bool PropertyGrid::draw( const nc::TypedPtr& p_value )
{
    const nc::types::TypeInfo* type = p_value.get_type();
    if (!type || !p_value.get_ptr() || !type->is_record())
        return false;

    nc::TypedPtr target = p_value;
    if (const nc::types::RecordInfo* dyn = resolve_record_info( type, p_value.get_ptr() ))
        target = nc::TypedPtr( dyn, p_value.get_ptr() );

    constexpr ImGuiTableFlags FLAGS =
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings;

    if (!ImGui::BeginTable( "##propgrid", 2, FLAGS ))
        return false;

    ImGui::TableSetupColumn( "Property", ImGuiTableColumnFlags_WidthFixed, LABEL_WIDTH );
    ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_WidthStretch );

    const bool changed = draw_record_body( target, 0 );

    ImGui::EndTable();
    return changed;
}

bool PropertyGrid::is_object_derived( const nc::types::TypeInfo* type )
{
    for (const nc::types::TypeInfo* t = type; t && t->is_record();) {
        if (t->name && ( std::strcmp( t->name, "Object" ) == 0 || std::strcmp( t->name, "nc::Object" ) == 0 ))
            return true;
        auto* rec = static_cast<const nc::types::RecordInfo*>( t );
        if (!rec->parent_id.valid())
            break;
        t = nc::TypeDB::find( rec->parent_id );
    }
    return false;
}

const nc::types::RecordInfo* PropertyGrid::resolve_record_info( const nc::types::TypeInfo* static_type, void* instance )
{
    if (!static_type || !static_type->is_record())
        return nullptr;
    auto* rec = static_cast<const nc::types::RecordInfo*>( static_type );
    if (instance && is_object_derived( static_type )) {
        auto* obj = static_cast<nc::Object*>( instance );
        return &obj->get_class_info();
    }
    return rec;
}

//------------------------------------------------------------------------------
// Rows
//------------------------------------------------------------------------------

void PropertyGrid::draw_guides( int depth )
{
    if (depth <= 0)
        return;
    ImDrawList* dl  = ImGui::GetWindowDrawList();
    const ImVec2 p  = ImGui::GetCursorScreenPos();
    const float pad = ImGui::GetStyle().CellPadding.y;
    const float h   = ImGui::GetFrameHeight() + pad * 2.0f;
    const ImU32 col = ImGui::GetColorU32( ImGuiCol_Separator, 0.7f );
    for (int d = 0; d < depth; ++d) {
        const float x = std::floor( p.x + static_cast<float>( d ) * INDENT_PX + 6.0f );
        dl->AddLine( ImVec2( x, p.y - pad ), ImVec2( x, p.y - pad + h ), col );
    }
}

void PropertyGrid::row_context()
{
    if (!menu)
        return;
    ElemMenu* m = menu;
    menu        = nullptr; // consume: only the first row of an element owns the menu

    if (ImGui::BeginPopupContextItem( "##elem" )) {
        if (ImGui::MenuItem( "Duplicate" )) {
            m->op       = OP_DUPLICATE;
            m->op_index = m->index;
        }
        if (ImGui::MenuItem( "Move Up", nullptr, false, m->index > 0 )) {
            m->op       = OP_MOVE_UP;
            m->op_index = m->index;
        }
        if (ImGui::MenuItem( "Move Down", nullptr, false, m->index + 1 < m->count )) {
            m->op       = OP_MOVE_DOWN;
            m->op_index = m->index;
        }
        ImGui::Separator();
        if (ImGui::MenuItem( "Remove" )) {
            m->op       = OP_REMOVE;
            m->op_index = m->index;
        }
        ImGui::EndPopup();
    }
}

void PropertyGrid::begin_prop_row( const char* label, int depth )
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex( 0 );
    ImGui::AlignTextToFramePadding();
    draw_guides( depth );
    ImGui::SetCursorPosX( ImGui::GetCursorPosX() + static_cast<float>( depth ) * INDENT_PX );
    ImGui::TextUnformatted( label );
    row_context();
    ImGui::TableSetColumnIndex( 1 );
    ImGui::AlignTextToFramePadding();
    ImGui::SetNextItemWidth( -FLT_MIN );
}

bool PropertyGrid::begin_tree_row( const char* label, int depth, ImGuiTreeNodeFlags flags )
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex( 0 );
    ImGui::AlignTextToFramePadding();
    draw_guides( depth );
    ImGui::SetCursorPosX( ImGui::GetCursorPosX() + static_cast<float>( depth ) * INDENT_PX );
    const bool open =
        ImGui::TreeNodeEx( label, flags | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth );
    row_context();
    ImGui::TableSetColumnIndex( 1 );
    ImGui::AlignTextToFramePadding();
    ImGui::SetNextItemWidth( -FLT_MIN );
    return open;
}

//------------------------------------------------------------------------------
// Records and fields
//------------------------------------------------------------------------------

bool PropertyGrid::draw_record_body( const nc::TypedPtr& rec, int depth )
{
    const nc::types::TypeInfo* t = rec.get_type();
    if (!t || !t->is_record() || !rec.get_ptr())
        return false;

    // fields() is per-class: walk the parent chain and draw base classes first.
    const nc::types::RecordInfo* chain[16];
    int n = 0;
    for (const nc::types::RecordInfo* r = static_cast<const nc::types::RecordInfo*>( t ); r && n < 16;) {
        chain[n++]                        = r;
        const nc::types::TypeInfo* parent = nc::TypeDB::find( r->parent_id );
        r = parent && parent->is_record() ? static_cast<const nc::types::RecordInfo*>( parent ) : nullptr;
    }

    bool changed = false;
    for (int i = n - 1; i >= 0; --i) {
        for (const auto& f : chain[i]->fields())
            changed |= draw_field( f, chain[i], rec, depth );
    }

    // General hook for any Material record (direct or via Ref<Material> expansion in
    // any component). This keeps the shader param UI aligned with normal properties
    // (using absolute cursor positioning in draw_material_params) without per-component
    // hardcoding in the panel. When the Material is the *primary* inspected thing
    // the MaterialInspectorPlugin (replaces_default) handles it via parse_end instead.
    if (assets) {
        const nc::types::RecordInfo* mat_rec = nc::TypeDB::find_record<nc::Material>();
        if (mat_rec && nc::TypeDB::is_derived_or_same( static_cast<const nc::types::RecordInfo*>( t ), mat_rec )) {
            auto* mat = static_cast<nc::Material*>( rec.get_ptr() );
            if (mat) {
                ImGui::Spacing();
                draw_material_params_grid( *this, *assets, *mat, depth );
            }
        }
    }

    return changed;
}

bool PropertyGrid::draw_field(
    const nc::types::FieldInfo& f, const nc::types::RecordInfo* declaring, const nc::TypedPtr& owner, int depth
)
{
    if (f.is( nc::types::PropertyFlags::HIDDEN ))
        return false;

    char label[96];
    utils::pretty_name( f.name, label, sizeof label );

    ImGui::PushID( f.name.data(), f.name.data() + f.name.size() );

    const bool read_only = f.is( nc::types::PropertyFlags::READ_ONLY );
    if (read_only)
        ImGui::BeginDisabled();

    // Godot-style per-field plugin hook (essentials only).
    // If any active plugin returns true, it handled (and drew) this field.
    bool plugin_handled = false;
    if (frame && !inspector_plugins.empty()) {
        for (auto* pl : inspector_plugins) {
            if (pl && pl->parse_property( *frame, declaring, owner.get_ptr(), f, label, depth )) {
                plugin_handled = true;
                break;
            }
        }
    }

    const nc::types::Qualifier& q = f.qualifier;
    void* slot                    = f.get_void_ptr( owner.get_ptr() );
    bool changed                  = false;

    if (!plugin_handled) {
        if (q.is_cstring) {
            begin_prop_row( label, depth );
            const char* s = f.get_as<const char*>( owner.get_ptr() );
            ImGui::TextDisabled( "%s", s ? s : "" );
        } else if (q.is_array()) {
            changed = draw_array( label, f.get_type(), slot, q.array_length, depth );
        } else if (q.is_pointer()) {
            changed = draw_value( label, owner.field( f ), depth, &f, declaring ); // null pointer -> "None"
        } else {
            changed = draw_value( label, nc::TypedPtr( f.get_type(), slot ), depth, &f, declaring );
        }
    }

    if (read_only)
        ImGui::EndDisabled();
    ImGui::PopID();
    return changed;
}

bool PropertyGrid::draw_value(
    const char* label, const nc::TypedPtr& v, int depth, const nc::types::FieldInfo* field,
    const nc::types::RecordInfo* declaring
)
{
    const nc::types::TypeInfo* type = v.get_type();
    void* ptr                       = v.get_ptr();

    if (!ptr) {
        begin_prop_row( label, depth );
        ImGui::TextDisabled( "None" );
        return false;
    }
    if (!type) {
        begin_prop_row( label, depth );
        ImGui::TextDisabled( "<unreflected type>" );
        return false;
    }

    // Registered drawers win over every built-in widget. READ_ONLY is enforced by
    // ImGui::BeginDisabled in draw_field, so `editable` is always valid: the drawers
    // dereference ctx.edit<T>() unguarded.
    if (drawers) {
        if (PropertyDrawerFn fn = drawers->find( type )) {
            begin_prop_row( label, depth );
            PropertyDrawContext ctx;
            ctx.value     = nc::ConstTypedPtr( v );
            ctx.editable  = v;
            ctx.field     = field;
            ctx.owner     = declaring;
            ctx.label     = nc::StringView( label );
            ctx.depth     = depth;
            ctx.read_only = false;
            ctx.frame     = frame;
            ctx.assets    = assets;
            return fn( ctx );
        }
    }

    if (type->is_primitive()) {
        begin_prop_row( label, depth );
        if (type->kind == nc::types::TypeKind::BOOL)
            return ImGui::Checkbox( "##v", static_cast<bool*>( ptr ) );
        const ImGuiDataType dt = imgui_data_type( type->kind );
        const bool fl          = type->is_floating();
        return ImGui::DragScalar( "##v", dt, ptr, fl ? 0.01f : 1.0f, nullptr, nullptr, fl ? "%.3f" : nullptr );
    }

    switch (type->kind) {
        case nc::types::TypeKind::ENUM: {
            begin_prop_row( label, depth );
            auto* e = static_cast<const nc::types::EnumInfo*>( type );
            if (!e || e->size == 0 || e->size > 8) {
                ImGui::TextDisabled( "<invalid enum>" );
                return false;
            }
            const int64_t cur = e->get_value( ptr );
            char cur_name[64];
            to_cstr( e->get_name( cur ), cur_name, sizeof cur_name );

            bool changed = false;
            if (ImGui::BeginCombo( "##v", cur_name )) {
                for (const auto& el : e->elements()) {
                    char nm[64];
                    to_cstr( el.name, nm, sizeof nm );
                    const bool sel = el.value == cur;
                    if (ImGui::Selectable( nm, sel )) {
                        e->set_value( ptr, el.value );
                        changed = true;
                    }
                    if (sel)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            return changed;
        }
        case nc::types::TypeKind::STRING: {
            begin_prop_row( label, depth );
            auto* s = static_cast<nc::String*>( ptr );
            return ImGui::InputText(
                "##v", s->data(), s->capacity() + 1, ImGuiInputTextFlags_CallbackResize, string_resize_callback, s
            );
        }
        case nc::types::TypeKind::REF:
            return draw_ref( label, v, depth );
        case nc::types::TypeKind::VECTOR:
            return draw_vector( label, v, depth );
        case nc::types::TypeKind::RECORD:
            return draw_record( label, v, depth );
        default:
            begin_prop_row( label, depth );
            ImGui::TextDisabled( "<%s>", type->name ? type->name : "?" );
            return false;
    }
}

bool PropertyGrid::draw_record( const char* label, const nc::TypedPtr& v, int depth )
{
    auto* rec = static_cast<const nc::types::RecordInfo*>( v.get_type() );
    void* ptr = v.get_ptr();

    InlineLayout L;
    if (inline_layout( rec, L )) {
        begin_prop_row( label, depth );
        return draw_compact( L, ptr );
    }

    if (depth >= MAX_DEPTH) {
        begin_prop_row( label, depth );
        ImGui::TextDisabled( "..." );
        return false;
    }

    const bool open = begin_tree_row( label, depth, depth == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0 );
    char type_name[64];
    utils::pretty_name( rec->name ? rec->name : "", type_name, sizeof type_name );
    ImGui::TextDisabled( "%s", type_name );

    return open ? draw_record_body( v, depth + 1 ) : false;
}

//------------------------------------------------------------------------------
// Containers
//------------------------------------------------------------------------------

bool PropertyGrid::draw_vector( const char* label, const nc::TypedPtr& v, int depth )
{
    auto* rec                       = static_cast<const nc::types::RecordInfo*>( v.get_type() );
    void* inst                      = v.get_ptr();
    const nc::types::TypeInfo* elem = rec->element_type();
    const size_t count              = rec->container_size( inst );

    if (!elem || depth >= MAX_DEPTH) {
        begin_prop_row( label, depth );
        ImGui::TextDisabled( elem ? "..." : "<unsupported container>" );
        return false;
    }

    const bool open = begin_tree_row( label, depth, depth == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0 );
    ImGui::TextDisabled( "%zu item%s", count, count == 1 ? "" : "s" );
    ImGui::SameLine();
    if (ImGui::SmallButton( "+##add" )) {
        rec->resize( inst, count + 1 );
        return true; // element pointers are stale, bail out of this frame
    }
    ImGui::SameLine();
    ImGui::BeginDisabled( count == 0 );
    const bool clear = ImGui::SmallButton( "Clear##clr" );
    ImGui::EndDisabled();
    if (clear) {
        rec->resize( inst, 0 );
        return true;
    }
    if (!open)
        return false;

    bool changed = false;

    if (elem->is_primitive() && count > MAX_PRIMITIVE_ROWS) {
        begin_prop_row( "", depth + 1 );
        ImGui::TextDisabled( "%zu elements (%zu bytes), not shown", count, count * elem->size );
        return false;
    }

    ElemMenu m;
    m.count            = count;
    const size_t shown = std::min( count, MAX_ROWS );
    auto* base         = static_cast<std::byte*>( rec->container_data( inst ) );

    for (size_t i = 0; i < shown; ++i) {
        ImGui::PushID( static_cast<int>( i ) );
        char lbl[24];
        std::snprintf( lbl, sizeof lbl, "[%zu]", i );
        m.index = i;
        menu    = &m;
        changed |= draw_value( lbl, nc::TypedPtr( elem, base + i * elem->size ), depth + 1 );
        menu = nullptr;
        ImGui::PopID();
    }

    if (count > shown) {
        begin_prop_row( "", depth + 1 );
        ImGui::TextDisabled( "... %zu more not shown", count - shown );
    }

    if (m.op != OP_NONE)
        changed |= apply_element_op( rec, inst, elem, count, m.op, m.op_index );
    return changed;
}

bool PropertyGrid::draw_array( const char* label, const nc::types::TypeInfo* elem, void* base, size_t count, int depth )
{
    if (!elem || !base || depth >= MAX_DEPTH) {
        begin_prop_row( label, depth );
        ImGui::TextDisabled( elem ? "..." : "<unreflected type>" );
        return false;
    }

    // float[2..4] / int32[2..4] -> one row, same widget as Vec2/3/4
    if (count >= 2 && count <= 4 && !( drawers && drawers->find( elem ) ) &&
        ( elem->kind == nc::types::TypeKind::FLOAT || elem->kind == nc::types::TypeKind::INT32 )) {
        begin_prop_row( label, depth );
        static const char* const FF[4] = { "X:%.3f", "Y:%.3f", "Z:%.3f", "W:%.3f" };
        static const char* const FI[4] = { "X:%d", "Y:%d", "Z:%d", "W:%d" };
        const int n                    = static_cast<int>( count );
        if (elem->kind == nc::types::TypeKind::FLOAT)
            return widgets::drag_float_components(
                static_cast<float*>( base ), widgets::DragComponentsDesc{ .count = n, .speed = 0.01f, .formats = FF }
            );
        return widgets::drag_int_components(
            static_cast<int*>( base ), widgets::DragComponentsDesc{ .count = n, .speed = 1.0f, .formats = FI }
        );
    }

    const bool open = begin_tree_row( label, depth, 0 );
    ImGui::TextDisabled( "[%zu]", count );
    if (!open)
        return false;

    bool changed       = false;
    const size_t shown = std::min( count, MAX_ROWS );
    nc::TypedPtr first( elem, base );
    for (size_t i = 0; i < shown; ++i) {
        ImGui::PushID( static_cast<int>( i ) );
        char lbl[24];
        std::snprintf( lbl, sizeof lbl, "[%zu]", i );
        changed |= draw_value( lbl, first.at( i ), depth + 1 );
        ImGui::PopID();
    }
    return changed;
}

//------------------------------------------------------------------------------
// Ref<T>
//------------------------------------------------------------------------------

bool PropertyGrid::assign_ref_path( const nc::types::IRefOps* ops, const nc::TypedPtr& slot, nc::StringView path )
{
    if (!assets || !ops)
        return false;
    nc::RefCounted* rc = assets->resolve( path );
    return rc && ops->share( slot.get_ptr(), rc );
}

bool PropertyGrid::pick_ref_popup( const nc::types::IRefOps* ops, const nc::TypedPtr& slot, const char* current_path )
{
    ImGui::SetNextWindowSizeConstraints( ImVec2( 300, 120 ), ImVec2( 520, 420 ) );
    if (!ImGui::BeginPopup( "##refpicker" ))
        return false;

    static char filter[64] = {}; // only one picker is open at a time
    static bool failed     = false;
    if (ImGui::IsWindowAppearing()) {
        filter[0] = '\0';
        failed    = false;
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::SetNextItemWidth( -FLT_MIN );
    ImGui::InputTextWithHint( "##flt", "Search assets...", filter, sizeof filter );

    bool changed = false;

    nc::DynamicArray<uint32_t> matches;
    if (assets) {
        uint32_t i = 0;
        for (const auto& e : assets->get_entries()) {
            if (icontains( e.path.c_str(), filter ))
                matches.push_back( i );
            ++i;
        }
    }

    if (ImGui::BeginChild( "##refpick_list", ImVec2( 0, 240 ), ImGuiChildFlags_Borders )) {
        if (ImGui::Selectable( "<None>", !current_path )) {
            ops->reset( slot.get_ptr() );
            changed = true;
            ImGui::CloseCurrentPopup();
        }

        if (assets) {
            auto entries = assets->get_entries();
            ImGuiListClipper clipper;
            clipper.Begin( static_cast<int>( matches.size() ) );
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                    const AssetEntry& e = entries[matches[static_cast<size_t>( row )]];
                    ImGui::PushID( row );

                    const bool sel = current_path && std::strcmp( e.path.c_str(), current_path ) == 0;
                    if (ImGui::Selectable( e.path.c_str(), sel )) {
                        if (assign_ref_path( ops, slot, nc::StringView( e.path ) )) {
                            changed = true;
                            ImGui::CloseCurrentPopup();
                        } else {
                            failed = true;
                        }
                    }

                    const nc::StringView kind = asset_kind_name( e.kind );
                    const float w             = ImGui::CalcTextSize( kind.data(), kind.data() + kind.size() ).x;
                    ImGui::SameLine( ImGui::GetWindowWidth() - w - 16.0f );
                    ImGui::TextDisabled( "%.*s", static_cast<int>( kind.size() ), kind.data() );
                    ImGui::PopID();
                }
            }
        }
    }
    ImGui::EndChild();

    if (failed)
        ImGui::TextColored( ImVec4( 1.0f, 0.4f, 0.35f, 1.0f ), "That asset can't be assigned to this slot." );

    ImGui::EndPopup();
    return changed;
}

bool PropertyGrid::draw_ref( const char* label, const nc::TypedPtr& slot, int depth )
{
    const nc::types::IRefOps* ops = slot.get_type()->as_ref();
    if (!ops) {
        begin_prop_row( label, depth );
        ImGui::TextDisabled( "<unsupported handle>" );
        return false;
    }

    nc::TypedPtr target   = slot.deref();
    nc::RefCounted* rc    = ops->get_refcounted( slot.get_ptr() );
    const bool has        = rc != nullptr;
    const bool expandable = target && target.get_type() && target.get_type()->is_record() && depth < MAX_DEPTH;

    const char* full_path = nullptr; // set when the pointee is a loaded resource
    const char* shown     = "None";
    if (has) {
        if (rc->is_a<nc::IResource>()) {
            full_path = static_cast<nc::IResource*>( rc )->filepath.c_str();
            shown     = file_part( full_path );
        } else {
            shown = target.get_type() && target.get_type()->name ? target.get_type()->name : "nc::Object";
        }
    }

    bool open = false;
    if (expandable)
        open = begin_tree_row( label, depth, 0 );
    else
        begin_prop_row( label, depth );

    // ---- value cell: [ name ............ ] [x] ---------------------------------
    const ImGuiStyle& st = ImGui::GetStyle();
    const float h        = ImGui::GetFrameHeight();
    const float btn_w    = std::max( 10.0f, ImGui::GetContentRegionAvail().x - ( has ? h + st.ItemSpacing.x : 0.0f ) );

    char text[320];
    std::snprintf( text, sizeof text, "%s##pick", shown );

    ImGui::PushStyleVar( ImGuiStyleVar_ButtonTextAlign, ImVec2( 0.0f, 0.5f ) );
    if (!has)
        ImGui::PushStyleColor( ImGuiCol_Text, ImGui::GetStyleColorVec4( ImGuiCol_TextDisabled ) );
    const bool clicked = ImGui::Button( text, ImVec2( btn_w, 0.0f ) );
    if (!has)
        ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (ImGui::IsItemHovered() && has)
        ImGui::SetTooltip( "%s\n%s", full_path ? full_path : shown, target.get_type() ? target.get_type()->name : "" );

    bool changed = false;

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload( ASSET_PAYLOAD )) {
            const char* d = static_cast<const char*>( p->Data );
            size_t len    = 0;
            while (len < static_cast<size_t>( p->DataSize ) && d[len])
                ++len;
            changed |= assign_ref_path( ops, slot, nc::StringView( d, len ) );
        }
        ImGui::EndDragDropTarget();
    }

    if (clicked)
        ImGui::OpenPopup( "##refpicker" );
    changed |= pick_ref_popup( ops, slot, full_path );

    if (has) {
        ImGui::SameLine();
        if (ImGui::Button( "x##clr", ImVec2( h, h ) )) {
            ops->reset( slot.get_ptr() );
            changed = true;
        }
        ImGui::SetItemTooltip( "Clear" );
    }

    // The handle may have been replaced above: don't touch the old pointee.
    if (open && !changed)
        changed |= draw_record_body( target, depth + 1 );

    return changed;
}

} // namespace sea::editor
