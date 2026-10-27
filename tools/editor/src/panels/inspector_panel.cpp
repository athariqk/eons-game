#include <algorithm>
#include <cstring>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <ncore/core/color.h>
#include <ncore/core/matrix.h>
#include <ncore/core/object.h>
#include <ncore/core/quaternion.h>
#include <ncore/core/rect.h>
#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/resources/resource.h>
#include <ncore/runtime/components/material.h>
#include <ncore/runtime/components/mesh.h>
#include <ncore/runtime/components/resource.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/video/gfx_types.h>
#include <ncore/utils/math.h>

#include <panels/inspector_panel.h>

namespace nc::editor {

InspectorPanel::InspectorPanel() : EditorPanel( "Inspector", DockSlot::RightUpper ) {}

void InspectorPanel::draw_node( EditorContext& ctx, Node& selected )
{
    auto& ecs = ctx.scene->get_ecs();

    const auto name = selected.get_name();
    const auto id   = selected.get_id();
    auto active     = selected.get_active();

    ImGui::Text( "Node: %s", name.data() );
    ImGui::Text( "ID: %llu", id );
    ImGui::Checkbox( "Active", active );
    ImGui::Separator();

    if (ImGui::Button( "+ Add Component", ImVec2( -FLT_MIN, 0 ) ))
        ImGui::OpenPopup( "AddComponent" );

    if (ImGui::BeginPopup( "AddComponent" )) {
        ImGui::InputTextWithHint( "##filter", "Search...", component_filter_, sizeof( component_filter_ ) );

        ImGui::BeginChild( "##complist", ImVec2( 0, 200 ), ImGuiChildFlags_Borders );
        for (auto type : ecs.get_component_types()) {
            if (!type->is_record() || is_internal_component( type ))
                continue;
            if (component_filter_[0] && !std::strstr( type->name, component_filter_ ))
                continue;

            const bool already = selected.has_component( type );
            ImGui::BeginDisabled( already );
            if (ImGui::Selectable( type->name )) {
                selected.add_component( type );
                component_filter_[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }

    // Apply after the loop: remove_component swaps the last entry in.
    DynamicArray<const rtti::TypeInfo*> to_remove;
    bool ensure_resource_tag = false;

    active_ctx_ = &ctx;

    for (auto& comp : selected.get_components()) {
        auto* type = ecs.resolve_component( comp.EcsId );
        if (!type || is_internal_component( type ))
            continue;

        void* data_ptr = selected.get_component( type );
        if (!data_ptr)
            continue;

        ImGui::Separator();
        if (!ImGui::CollapsingHeader( type->name, ImGuiTreeNodeFlags_DefaultOpen ))
            continue;

        ImGui::PushID( static_cast<int>( id << 16 | type->id.value ) );

        // Inspector handlers may take over the sheet entirely (e.g. a future
        // sprite editor); otherwise the generic sheet draws first and handlers
        // append their own sections.
        bool replaces_default = false;
        if (ctx.inspector_handlers) {
            for (const auto& handler : *ctx.inspector_handlers) {
                if (handler && handler->can_handle( type, data_ptr ) && handler->replaces_default()) {
                    replaces_default = true;
                    break;
                }
            }
        }

        if (!replaces_default && draw_property( type, data_ptr )) {
            selected.notify_changed( type );

            // Re-pointed resource handles only re-init on the ResourceLoaded path,
            // which requires the tag.
            if (( type == rtti::TypeRegistry::find<MeshComponent>() ||
                  type == rtti::TypeRegistry::find<MaterialComponent>() ) &&
                !selected.has_component<HasResourceTag>()) {
                ensure_resource_tag = true;
            }
        }

        if (ctx.inspector_handlers) {
            for (const auto& handler : *ctx.inspector_handlers) {
                if (handler)
                    handler->draw( ctx, selected, type, data_ptr );
            }
        }

        if (ImGui::SmallButton( "Remove" ))
            to_remove.push_back( type );

        ImGui::BeginDisabled( !comp.Toggleable );
        ImGui::Checkbox( "Active", &comp.Active );
        ImGui::EndDisabled();
        ImGui::PopID();
    }

    active_ctx_ = nullptr;

    for (auto* type : to_remove)
        selected.remove_component( type );

    if (ensure_resource_tag)
        selected.add_component<HasResourceTag>();
}

void InspectorPanel::draw( EditorContext& ctx )
{
    if (!ImGui::Begin( title.c_str(), &is_visible )) {
        ImGui::End();
        return;
    }

    Node* selected = ( ctx.scene && ctx.selection_state ) ? ctx.selection_state->node( *ctx.scene ) : nullptr;

    if (!selected)
        ImGui::TextDisabled( "No node get_selected_eid" );
    else
        draw_node( ctx, *selected );

    ImGui::End();
}

} // namespace nc::editor

namespace nc::editor {

/**
 * @brief Widget registry wrapper threaded through the property recursion.
 *
 * Widgets are matched by reflected
 * type; the first match owns the row.
 */
struct PropertyGridWidgets {
    const DynamicArray<PropertyWidget>* List = nullptr;
    EditorContext* ctx                       = nullptr;

    bool handles( const rtti::TypeInfo* type ) const
    {
        if (!List)
            return false;
        for (const auto& widget : *List) {
            if (widget.Type == type)
                return true;
        }
        return false;
    }

    /** @brief Returns true when a widget owned the value; out_changed reports edits. */
    bool draw(
        const rtti::TypeInfo* type, const rtti::RecordInfo* owner, StringView field, void* value, bool& out_changed
    ) const
    {
        out_changed = false;
        if (!List || !ctx)
            return false;

        for (const auto& widget : *List) {
            if (widget.Type != type || !widget.Draw)
                continue;
            return widget.Draw( *ctx, owner, field, type, value, out_changed );
        }
        return false;
    }
};

static ImGuiDataType imgui_data_type( rtti::TypeKind kind )
{
    switch (kind) {
        case rtti::TypeKind::INT8:
            return ImGuiDataType_S8;
        case rtti::TypeKind::UINT8:
            return ImGuiDataType_U8;
        case rtti::TypeKind::INT16:
            return ImGuiDataType_S16;
        case rtti::TypeKind::UINT16:
            return ImGuiDataType_U16;
        case rtti::TypeKind::INT32:
            return ImGuiDataType_S32;
        case rtti::TypeKind::UINT32:
            return ImGuiDataType_U32;
        case rtti::TypeKind::INT64:
            return ImGuiDataType_S64;
        case rtti::TypeKind::UINT64:
            return ImGuiDataType_U64;
        case rtti::TypeKind::FLOAT:
            return ImGuiDataType_Float;
        case rtti::TypeKind::DOUBLE:
            return ImGuiDataType_Double;
        default:
            return ImGuiDataType_COUNT;
    }
}

static int string_resize_callback( ImGuiInputTextCallbackData* cb )
{
    if (cb->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* s = static_cast<String*>( cb->UserData );
        s->resize( static_cast<size_t>( cb->BufTextLen ) );
        cb->Buf = s->data();
    }
    return 0;
}

template<typename T>
static bool is_type( const rtti::TypeInfo* type )
{
    return type && type == rtti::TypeRegistry::find<T>();
}

// Multi-component float drag with XYZ colour coding (Unity-style).
static bool drag_floats_colored( float* v, int components, float speed = 0.01f )
{
    static const ImVec4 kAxis[4] = {
        ImVec4( 0.92f, 0.28f, 0.28f, 1.0f ),
        ImVec4( 0.32f, 0.82f, 0.32f, 1.0f ),
        ImVec4( 0.32f, 0.55f, 0.95f, 1.0f ),
        ImVec4( 0.85f, 0.85f, 0.35f, 1.0f ),
    };
    static const char* kFmt[4] = { "X:%.3f", "Y:%.3f", "Z:%.3f", "W:%.3f" };

    bool changed        = false;
    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    const float total   = ImGui::CalcItemWidth();
    const float cell    = ( total - spacing * static_cast<float>( components - 1 ) ) / static_cast<float>( components );

    for (int i = 0; i < components; ++i) {
        if (i > 0)
            ImGui::SameLine( 0.0f, spacing );
        ImGui::PushID( i );
        ImGui::PushStyleColor( ImGuiCol_Border, kAxis[i] );
        ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 1.5f );
        ImGui::SetNextItemWidth( cell );
        changed |= ImGui::DragFloat( "##c", &v[i], speed, 0.0f, 0.0f, kFmt[i] );
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
    return changed;
}

static bool drag_ints_colored( int* v, int components, float speed = 1.0f )
{
    static const ImVec4 kAxis[2] = {
        ImVec4( 0.92f, 0.28f, 0.28f, 1.0f ),
        ImVec4( 0.32f, 0.82f, 0.32f, 1.0f ),
    };
    static const char* kFmt[2] = { "X:%d", "Y:%d" };

    bool changed        = false;
    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    const float total   = ImGui::CalcItemWidth();
    const float cell    = ( total - spacing * static_cast<float>( components - 1 ) ) / static_cast<float>( components );

    for (int i = 0; i < components; ++i) {
        if (i > 0)
            ImGui::SameLine( 0.0f, spacing );
        ImGui::PushID( i );
        ImGui::PushStyleColor( ImGuiCol_Border, kAxis[i] );
        ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 1.5f );
        ImGui::SetNextItemWidth( cell );
        changed |= ImGui::DragInt( "##c", &v[i], speed, 0, 0, kFmt[i] );
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
    return changed;
}

static bool draw_vec2f( void* ptr )
{
    return drag_floats_colored( static_cast<Vec2f*>( ptr )->data(), 2 );
}

static bool draw_vec2i( void* ptr )
{
    return drag_ints_colored( static_cast<Vec2i*>( ptr )->data(), 2 );
}

static bool draw_vec3( void* ptr )
{
    return drag_floats_colored( static_cast<Vec3f*>( ptr )->data(), 3 );
}

static bool draw_vec4( void* ptr )
{
    return drag_floats_colored( static_cast<Vec4f*>( ptr )->data(), 4 );
}

static bool draw_rect2f( void* ptr )
{
    auto* r      = static_cast<Rect2f*>( ptr );
    bool changed = false;

    float pos[2] = { r->x, r->y };
    float sz[2]  = { r->w, r->h };

    // Pos (X/Y)
    ImGui::PushID( "pos" );
    ImGui::SetNextItemWidth( -FLT_MIN );
    if (drag_floats_colored( pos, 2 )) {
        r->x    = pos[0];
        r->y    = pos[1];
        changed = true;
    }
    ImGui::PopID();

    // Size (W/H) — same coloured style, custom format
    {
        ImGui::PushID( "size" );
        static const ImVec4 kAxis[2] = {
            ImVec4( 0.85f, 0.55f, 0.15f, 1.0f ), // W orange
            ImVec4( 0.55f, 0.35f, 0.85f, 1.0f ), // H purple
        };
        static const char* kFmt[2] = { "W:%.3f", "H:%.3f" };
        const float spacing        = ImGui::GetStyle().ItemInnerSpacing.x;
        const float total          = ImGui::CalcItemWidth();
        const float cell           = ( total - spacing ) * 0.5f;
        for (int i = 0; i < 2; ++i) {
            if (i > 0)
                ImGui::SameLine( 0.0f, spacing );
            ImGui::PushID( i );
            ImGui::PushStyleColor( ImGuiCol_Border, kAxis[i] );
            ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 1.5f );
            ImGui::SetNextItemWidth( cell );
            if (ImGui::DragFloat( "##c", &sz[i], 0.01f, 0.0f, 0.0f, kFmt[i] )) {
                r->w    = sz[0];
                r->h    = sz[1];
                changed = true;
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::PopID();
    }
    return changed;
}

static bool draw_rect2i( void* ptr )
{
    auto* r      = static_cast<Rect2i*>( ptr );
    bool changed = false;

    int pos[2] = { r->x, r->y };
    int sz[2]  = { r->w, r->h };

    ImGui::PushID( "pos" );
    ImGui::SetNextItemWidth( -FLT_MIN );
    if (drag_ints_colored( pos, 2 )) {
        r->x    = pos[0];
        r->y    = pos[1];
        changed = true;
    }
    ImGui::PopID();

    {
        ImGui::PushID( "size" );
        static const ImVec4 kAxis[2] = {
            ImVec4( 0.85f, 0.55f, 0.15f, 1.0f ),
            ImVec4( 0.55f, 0.35f, 0.85f, 1.0f ),
        };
        static const char* kFmt[2] = { "W:%d", "H:%d" };
        const float spacing        = ImGui::GetStyle().ItemInnerSpacing.x;
        const float total          = ImGui::CalcItemWidth();
        const float cell           = ( total - spacing ) * 0.5f;
        for (int i = 0; i < 2; ++i) {
            if (i > 0)
                ImGui::SameLine( 0.0f, spacing );
            ImGui::PushID( i );
            ImGui::PushStyleColor( ImGuiCol_Border, kAxis[i] );
            ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 1.5f );
            ImGui::SetNextItemWidth( cell );
            if (ImGui::DragInt( "##c", &sz[i], 1.0f, 0, 0, kFmt[i] )) {
                r->w    = sz[0];
                r->h    = sz[1];
                changed = true;
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::PopID();
    }
    return changed;
}

inline bool draw_rid( void* ptr )
{
    auto* rid = static_cast<RID*>( ptr );
    if (!rid->is_valid()) {
        ImGui::TextDisabled( "Invalid" );
        return false;
    }

    const uint32_t index = static_cast<uint32_t>( rid->value & 0xFFFFFFFFu );
    const uint32_t gen   = static_cast<uint32_t>( rid->value >> 32 );

    // Compact: idx:gen  + hex. RID is an opaque handle — edit raw value only.
    ImGui::Text( "%u:%u", index, gen );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( -FLT_MIN );
    bool changed = ImGui::InputScalar(
        "##rid", ImGuiDataType_U64, &rid->value, nullptr, nullptr, "0x%llX", ImGuiInputTextFlags_CharsHexadecimal
    );
    return changed;
}

static bool draw_color( void* ptr )
{
    auto* c       = static_cast<Color*>( ptr );
    float rgba[4] = { c->r / 255.0f, c->g / 255.0f, c->b / 255.0f, c->a / 255.0f };
    bool changed  = ImGui::ColorEdit4(
        "##col", rgba, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB
    );
    if (changed) {
        c->r = static_cast<uint8_t>( std::lround( rgba[0] * 255.0f ) );
        c->g = static_cast<uint8_t>( std::lround( rgba[1] * 255.0f ) );
        c->b = static_cast<uint8_t>( std::lround( rgba[2] * 255.0f ) );
        c->a = static_cast<uint8_t>( std::lround( rgba[3] * 255.0f ) );
    }
    return changed;
}

inline bool draw_quaternion( void* ptr )
{
    auto* q      = static_cast<Quaternion*>( ptr );
    bool changed = false;

    // Per-widget mode lives in ImGui storage keyed by ID stack.
    ImGuiStorage* storage = ImGui::GetStateStorage();
    ImGuiID mode_id       = ImGui::GetID( "##quat_mode" );
    int mode              = storage->GetInt( mode_id, 0 ); // 0 = Euler, 1 = Raw

    // Mode toggle (compact).
    {
        const char* labels[] = { "Euler", "Raw" };
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4.0f, 0.0f ) );
        for (int i = 0; i < 2; ++i) {
            if (i > 0)
                ImGui::SameLine();
            bool selected = ( mode == i );
            if (selected)
                ImGui::PushStyleColor( ImGuiCol_Button, ImGui::GetStyleColorVec4( ImGuiCol_ButtonActive ) );
            if (ImGui::SmallButton( labels[i] )) {
                mode = i;
                storage->SetInt( mode_id, mode );
            }
            if (selected)
                ImGui::PopStyleColor();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton( "I" )) { // Identity
            *q      = Quaternion::identity();
            changed = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip( "Identity" );
        ImGui::SameLine();
        if (ImGui::SmallButton( "N" )) { // Normalize
            *q      = Quaternion::normalize( *q );
            changed = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip( "Normalize" );
        ImGui::PopStyleVar();
    }

    ImGui::SetNextItemWidth( -FLT_MIN );

    if (mode == 0) {
        // --- Euler degrees (ZYX sequence, matches Quaternion(Vec3f) ctor) ---
        Vec3f euler_rad = Quaternion::to_euler_angles( *q );
        float deg[3]    = {
            math::rad_to_deg( euler_rad.x ),
            math::rad_to_deg( euler_rad.y ),
            math::rad_to_deg( euler_rad.z ),
        };

        if (drag_floats_colored( deg, 3, 0.5f )) {
            *q = Quaternion(
                Vec3f( math::deg_to_rad( deg[0] ), math::deg_to_rad( deg[1] ), math::deg_to_rad( deg[2] ) )
            );
            changed = true;
        }
    } else {
        // --- Raw w + v.xyz ---
        float raw[4] = { q->w, q->v.x, q->v.y, q->v.z };
        // W uses a neutral border; XYZ keep axis colours via a small custom pass.
        static const ImVec4 kAxis[4] = {
            ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ), // W grey
            ImVec4( 0.92f, 0.28f, 0.28f, 1.0f ), // X
            ImVec4( 0.32f, 0.82f, 0.32f, 1.0f ), // Y
            ImVec4( 0.32f, 0.55f, 0.95f, 1.0f ), // Z
        };
        static const char* kFmt[4] = { "W:%.4f", "X:%.4f", "Y:%.4f", "Z:%.4f" };

        const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
        const float total   = ImGui::CalcItemWidth();
        const float cell    = ( total - spacing * 3.0f ) / 4.0f;
        bool raw_changed    = false;

        for (int i = 0; i < 4; ++i) {
            if (i > 0)
                ImGui::SameLine( 0.0f, spacing );
            ImGui::PushID( i );
            ImGui::PushStyleColor( ImGuiCol_Border, kAxis[i] );
            ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 1.5f );
            ImGui::SetNextItemWidth( cell );
            raw_changed |= ImGui::DragFloat( "##q", &raw[i], 0.001f, 0.0f, 0.0f, kFmt[i] );
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::PopID();
        }

        if (raw_changed) {
            q->w    = raw[0];
            q->v.x  = raw[1];
            q->v.y  = raw[2];
            q->v.z  = raw[3];
            changed = true;
        }
    }

    return changed;
}

static bool draw_mat3( void* ptr )
{
    auto* m      = static_cast<Mat3*>( ptr );
    bool changed = false;
    ImGui::PushID( "mat3" );
    for (int row = 0; row < 3; ++row) {
        float row_vals[3] = { m->col0.data()[row], m->col1.data()[row], m->col2.data()[row] };
        ImGui::PushID( row );
        ImGui::SetNextItemWidth( -FLT_MIN );
        if (ImGui::DragFloat3( "##row", row_vals, 0.01f )) {
            m->col0.data()[row] = row_vals[0];
            m->col1.data()[row] = row_vals[1];
            m->col2.data()[row] = row_vals[2];
            changed             = true;
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    return changed;
}

static bool draw_mat4( void* ptr )
{
    auto* m      = static_cast<Mat4*>( ptr );
    bool changed = false;
    ImGui::PushID( "mat4" );
    for (int row = 0; row < 4; ++row) {
        float row_vals[4] = { m->col0.data()[row], m->col1.data()[row], m->col2.data()[row], m->col3.data()[row] };
        ImGui::PushID( row );
        ImGui::SetNextItemWidth( -FLT_MIN );
        if (ImGui::DragFloat4( "##row", row_vals, 0.01f )) {
            m->col0.data()[row] = row_vals[0];
            m->col1.data()[row] = row_vals[1];
            m->col2.data()[row] = row_vals[2];
            m->col3.data()[row] = row_vals[3];
            changed             = true;
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    return changed;
}

static bool draw_specialized( const rtti::TypeInfo* type, void* ptr, bool& out_changed )
{
    out_changed = false;
    if (!type || !ptr)
        return false;

    if (is_type<Vec2f>( type )) {
        out_changed = draw_vec2f( ptr );
        return true;
    }
    if (is_type<Vec2i>( type )) {
        out_changed = draw_vec2i( ptr );
        return true;
    }
    if (is_type<Vec3f>( type )) {
        out_changed = draw_vec3( ptr );
        return true;
    }
    if (is_type<Vec4f>( type )) {
        out_changed = draw_vec4( ptr );
        return true;
    }
    if (is_type<Color>( type )) {
        out_changed = draw_color( ptr );
        return true;
    }
    if (is_type<Quaternion>( type )) {
        out_changed = draw_quaternion( ptr );
        return true;
    }
    if (is_type<RID>( type )) {
        out_changed = draw_rid( ptr );
        return true;
    }
    if (is_type<Rect2f>( type )) {
        out_changed = draw_rect2f( ptr );
        return true;
    }
    if (is_type<Rect2i>( type )) {
        out_changed = draw_rect2i( ptr );
        return true;
    }
    if (is_type<Mat3>( type )) {
        out_changed = draw_mat3( ptr );
        return true;
    }
    if (is_type<Mat4>( type )) {
        out_changed = draw_mat4( ptr );
        return true;
    }
    return false;
}

static bool draw_leaf_value( const rtti::TypeInfo* type, void* ptr )
{
    switch (type->kind) {
        case rtti::TypeKind::BOOL:
            return ImGui::Checkbox( "##v", static_cast<bool*>( ptr ) );

        case rtti::TypeKind::INT8:
        case rtti::TypeKind::UINT8:
        case rtti::TypeKind::INT16:
        case rtti::TypeKind::UINT16:
        case rtti::TypeKind::INT32:
        case rtti::TypeKind::UINT32:
        case rtti::TypeKind::INT64:
        case rtti::TypeKind::UINT64:
            return ImGui::DragScalar( "##v", imgui_data_type( type->kind ), ptr, 1.0f );

        case rtti::TypeKind::FLOAT:
        case rtti::TypeKind::DOUBLE:
            return ImGui::DragScalar( "##v", imgui_data_type( type->kind ), ptr, 0.01f );

        case rtti::TypeKind::ENUM: {
            auto* info       = static_cast<const rtti::EnumInfo*>( type );
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

        case rtti::TypeKind::STRING: {
            auto* str = static_cast<String*>( ptr );
            return ImGui::InputText(
                "##v", str->data(), str->capacity() + 1, ImGuiInputTextFlags_CallbackResize, string_resize_callback, str
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

static void begin_prop_row( StringView label, int depth )
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex( 0 );

    if (depth > 0) {
        ImGui::Dummy( ImVec2( InspectorPanel::kIndentPx * static_cast<float>( depth ), 0.0f ) );
        ImGui::SameLine( 0.0f, 0.0f );
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted( label.data(), label.data() + label.size() );

    ImGui::TableSetColumnIndex( 1 );
    ImGui::SetNextItemWidth( -FLT_MIN );
}

static bool
draw_record_fields( const rtti::RecordInfo* record, void* data, int depth, const PropertyGridWidgets* widgets );
static bool draw_labeled_value(
    StringView label, const rtti::TypeInfo* type, void* ptr, int depth, const PropertyGridWidgets* widgets
);
static bool draw_field(
    const rtti::FieldInfo& field, const rtti::RecordInfo* owner, void* data, int depth,
    const PropertyGridWidgets* widgets
);

inline bool is_object_derived( const rtti::TypeInfo* type )
{
    for (const rtti::TypeInfo* t = type; t && t->is_record();) {
        if (t->name && ( std::strcmp( t->name, "Object" ) == 0 || std::strcmp( t->name, "nc::Object" ) == 0 ))
            return true;
        auto* rec = static_cast<const rtti::RecordInfo*>( t );
        if (!rec->parent_id.valid())
            break;
        t = rtti::TypeRegistry::find( rec->parent_id );
    }
    return false;
}

// Resolve the most-derived rtti::RecordInfo for a live instance when possible.
inline const rtti::RecordInfo* resolve_record_info( const rtti::TypeInfo* static_type, void* instance )
{
    if (!static_type || !static_type->is_record())
        return nullptr;
    auto* rec = static_cast<const rtti::RecordInfo*>( static_type );
    if (instance && is_object_derived( static_type )) {
        auto* obj = static_cast<Object*>( instance );
        return &obj->get_class_info();
    }
    return rec;
}

// Draw a pointed-to record in the property table (label column already has TreeNode intent).
inline bool draw_pointee_record(
    StringView label, const rtti::TypeInfo* pointee_type, void* target, int depth, const PropertyGridWidgets* widgets
)
{
    if (!target) {
        begin_prop_row( label, depth );
        const char* tname = ( pointee_type && pointee_type->name ) ? pointee_type->name : "void";
        ImGui::TextDisabled( "%s*  null", tname );
        return false;
    }

    const rtti::RecordInfo* rec = resolve_record_info( pointee_type, target );
    if (!rec) {
        begin_prop_row( label, depth );
        const char* tname = ( pointee_type && pointee_type->name ) ? pointee_type->name : "void";
        ImGui::TextDisabled( "%s*  %p", tname, target );
        return false;
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex( 0 );
    if (depth > 0) {
        ImGui::Dummy( ImVec2( InspectorPanel::kIndentPx * static_cast<float>( depth ), 0.0f ) );
        ImGui::SameLine( 0.0f, 0.0f );
    }

    const char* tname = rec->name ? rec->name : "record";
    std::string lbl   = std::string( label ) + "  (" + tname + ")";
    constexpr ImGuiTreeNodeFlags kFlags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    bool open = ImGui::TreeNodeEx( lbl.c_str(), kFlags );

    ImGui::TableSetColumnIndex( 1 );
    ImGui::TextDisabled( "%p", target );

    if (open && depth < 8)
        return draw_record_fields( rec, target, depth + 1, widgets );
    return false;
}

struct PropGridVisitor : rtti::RecordVisitor {
    int base_depth                       = 0;
    bool changed                         = false;
    int cur_index                        = -1;
    int array_depth                      = 0;
    const rtti::FieldInfo* pending_field = nullptr;
    int pending_depth                    = 0;
    const PropertyGridWidgets* widgets   = nullptr;

    void class_begin( const rtti::RecordInfo* c, int depth ) override
    {
        if (cur_index < 0 || !c)
            return;
        // Element is a record — header row with [i] TypeName
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );
        const int d = base_depth + 1;
        if (d > 0) {
            ImGui::Dummy( ImVec2( InspectorPanel::kIndentPx * static_cast<float>( d ), 0.0f ) );
            ImGui::SameLine( 0.0f, 0.0f );
        }
        std::string lbl = "[" + std::to_string( cur_index ) + "] " + ( c->name ? c->name : "record" );
        constexpr ImGuiTreeNodeFlags kFlags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        ImGui::TreeNodeEx( lbl.c_str(), kFlags );
        ImGui::TableSetColumnIndex( 1 );
        ( void ) depth;
    }
    void class_end( const rtti::RecordInfo*, int ) override {}
    void class_member( const rtti::FieldInfo* f, int depth ) override
    {
        pending_field = f;
        pending_depth = depth;
    }

    void array_begin( const rtti::TypeInfo* t, const rtti::RecordInfo*, void*, int depth, int length ) override
    {
        array_depth = depth;
        cur_index   = -1;
        ( void ) t;
        ( void ) length;
    }
    void array_end( const rtti::TypeInfo*, int ) override
    {
        cur_index = -1;
    }
    void array_element( const rtti::TypeInfo*, int, int elem ) override
    {
        cur_index     = elem;
        pending_field = nullptr;
    }

    void primitive( const rtti::TypeInfo* t, void* instance ) override
    {
        if (!t || !instance)
            return;

        if (pending_field) {
            StringView n{ pending_field->name.data(), pending_field->name.size() };
            // Indent under the element record header
            const int d = base_depth + 2;
            changed |= draw_labeled_value( n, t, instance, d, widgets );
            pending_field = nullptr;
            return;
        }
        if (cur_index >= 0) {
            std::string idx = "[" + std::to_string( cur_index ) + "]";
            changed |= draw_labeled_value( idx, t, instance, base_depth + 1, widgets );
            return;
        }
        changed |= draw_leaf_value( t, instance );
    }
    void string( const rtti::TypeInfo* t, void* instance ) override
    {
        primitive( t, instance );
    }
};

static bool draw_labeled_value(
    StringView label, const rtti::TypeInfo* type, void* ptr, int depth, const PropertyGridWidgets* widgets
)
{
    if (!type) {
        begin_prop_row( label, depth );
        ImGui::TextDisabled( "<unregistered type>" );
        return false;
    }

    if (type->kind == rtti::TypeKind::RECORD) {
        if (is_type<Vec2f>( type ) || is_type<Vec2i>( type ) || is_type<Vec3f>( type ) || is_type<Vec4f>( type ) ||
            is_type<Color>( type ) || is_type<Quaternion>( type ) || is_type<Mat3>( type ) || is_type<Mat4>( type ) ||
            is_type<Rect2f>( type ) || is_type<Rect2i>( type ) || is_type<RID>( type )) {
            begin_prop_row( label, depth );
            bool changed = false;
            draw_specialized( type, ptr, changed );
            return changed;
        }

        if (depth >= 8) {
            begin_prop_row( label, depth );
            ImGui::TextDisabled( "<max depth>" );
            return false;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );
        if (depth > 0) {
            ImGui::Dummy( ImVec2( InspectorPanel::kIndentPx * static_cast<float>( depth ), 0.0f ) );
            ImGui::SameLine( 0.0f, 0.0f );
        }

        String lbl( label );
        constexpr ImGuiTreeNodeFlags kFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        bool open                           = ImGui::TreeNodeEx( lbl.c_str(), kFlags );

        ImGui::TableSetColumnIndex( 1 );

        if (open)
            return draw_record_fields( static_cast<const rtti::RecordInfo*>( type ), ptr, depth + 1, widgets );
        return false;
    } else if (type->kind == rtti::TypeKind::VECTOR) {
        auto* rec       = static_cast<const rtti::RecordInfo*>( type );
        std::string lbl = std::string( label );

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );
        if (depth > 0) {
            ImGui::Dummy( ImVec2( InspectorPanel::kIndentPx * static_cast<float>( depth ), 0.0f ) );
            ImGui::SameLine( 0.0f, 0.0f );
        }

        constexpr ImGuiTreeNodeFlags kFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        bool open                           = ImGui::TreeNodeEx( lbl.c_str(), kFlags );

        ImGui::TableSetColumnIndex( 1 );
        // std::vector / DynamicArray layout: { T* start, T* finish, T* end_cap }
        struct VectorRaw {
            void* start;
            void* finish;
            void* end_cap;
        };
        auto* vr          = static_cast<VectorRaw*>( ptr );
        const char* tname = type->name ? type->name : "vector";
        if (!vr->start || vr->start == vr->finish) {
            ImGui::TextDisabled( "%s  (empty)", tname );
        } else {
            ImGui::TextDisabled( "%s", tname );
        }

        if (open) {
            PropGridVisitor visitor;
            visitor.base_depth = depth;
            visitor.widgets    = widgets;
            rec->visit( ptr, &visitor, static_cast<rtti::PropertyFlags>( 0xFFFF ), static_cast<unsigned>( depth ) );
            return visitor.changed;
        }
        return false;
    }

    begin_prop_row( label, depth );
    return draw_leaf_value( type, ptr );
}

static bool draw_field(
    const rtti::FieldInfo& field, const rtti::RecordInfo* owner, void* data, int depth,
    const PropertyGridWidgets* widgets
)
{
    if (field.is( rtti::PropertyFlags::HIDDEN ))
        return false;

    const rtti::TypeInfo* type = field.get_type();
    StringView name{ field.name.data(), field.name.size() };
    void* ptr = field.get_void_ptr( data );

    bool changed = false;

    const bool read_only = field.is( rtti::PropertyFlags::READ_ONLY ) || !field.is( rtti::PropertyFlags::EDITABLE );

    ImGui::PushID( name.data(), name.data() + name.size() );
    ImGui::BeginDisabled( read_only );

    if (field.qualifier.is_pointer()) {
        void* target = field.get_as<void*>( data );

        if (field.qualifier.is_cstring) {
            begin_prop_row( name, depth );
            const char* s = static_cast<const char*>( target );
            if (!s)
                ImGui::TextDisabled( "(null)" );
            else
                ImGui::TextUnformatted( s );
        } else if (type && type->is_record()) {
            changed = draw_pointee_record( name, type, target, depth, widgets );
        } else {
            begin_prop_row( name, depth );
            const char* tname = ( type && type->name ) ? type->name : "void";
            if (!target)
                ImGui::TextDisabled( "%s*  null", tname );
            else
                ImGui::TextDisabled( "%s*  %p", tname, target );
        }
    } else if (field.qualifier.is_array()) {
        const uint32_t len = field.qualifier.array_length;
        std::string lbl    = std::string( name ) + " [" + std::to_string( len ) + "]";

        const bool inline_prim = type && type->is_primitive() && len > 0 && len <= 4 && depth < 6;

        if (inline_prim) {
            begin_prop_row( name, depth );
            auto* base          = static_cast<uint8_t*>( ptr );
            const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
            const float total   = ImGui::CalcItemWidth();
            const float cell    = ( total - spacing * static_cast<float>( len - 1 ) ) / static_cast<float>( len );
            for (uint32_t i = 0; i < len; ++i) {
                if (i > 0)
                    ImGui::SameLine( 0.0f, spacing );
                ImGui::PushID( static_cast<int>( i ) );
                ImGui::SetNextItemWidth( cell );
                changed |= draw_leaf_value( type, base + i * type->size );
                ImGui::PopID();
            }
        } else {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex( 0 );
            if (depth > 0) {
                ImGui::Dummy( ImVec2( InspectorPanel::kIndentPx * static_cast<float>( depth ), 0.0f ) );
                ImGui::SameLine( 0.0f, 0.0f );
            }

            constexpr ImGuiTreeNodeFlags kFlags =
                ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            bool open = type && ImGui::TreeNodeEx( lbl.c_str(), kFlags );

            ImGui::TableSetColumnIndex( 1 );
            if (type)
                ImGui::TextDisabled( "%u elements", len );

            if (open && type) {
                auto* base              = static_cast<uint8_t*>( ptr );
                constexpr uint32_t kMax = 64;
                const uint32_t show     = len < kMax ? len : kMax;
                const bool use_widget   = widgets && widgets->handles( type );
                for (uint32_t i = 0; i < show; ++i) {
                    ImGui::PushID( static_cast<int>( i ) );
                    std::string idx = "[" + std::to_string( i ) + "]";
                    if (use_widget) {
                        begin_prop_row( idx, depth + 1 );
                        bool widget_changed = false;
                        if (widgets->draw( type, owner, name, base + i * type->size, widget_changed ))
                            changed |= widget_changed;
                        else
                            changed |= draw_leaf_value( type, base + i * type->size );
                    } else {
                        changed |= draw_labeled_value( idx, type, base + i * type->size, depth + 1, widgets );
                    }
                    ImGui::PopID();
                }
                if (len > kMax) {
                    begin_prop_row( "...", depth + 1 );
                    ImGui::TextDisabled( "%u more elements", len - kMax );
                }
            }
        }
    } else if (widgets && widgets->handles( type )) {
        begin_prop_row( name, depth );
        bool widget_changed = false;
        if (widgets->draw( type, owner, name, ptr, widget_changed ))
            changed = widget_changed;
        else if (is_type<RID>( type ))
            changed = draw_rid( ptr );
        else
            changed = draw_leaf_value( type, ptr );
    } else {
        changed = draw_labeled_value( name, type, ptr, depth, widgets );
    }

    ImGui::EndDisabled();
    ImGui::PopID();
    return changed;
}

static bool
draw_record_fields( const rtti::RecordInfo* record, void* data, int depth, const PropertyGridWidgets* widgets )
{
    bool changed = false;
    for (const auto& field : record->fields())
        changed |= draw_field( field, record, data, depth, widgets );
    return changed;
}

bool InspectorPanel::draw_property( const rtti::TypeInfo* type, void* data )
{
    NC_ASSERT( type->is_record() );

    auto record = static_cast<const rtti::RecordInfo*>( type );
    if (record->field_count() == 0)
        return false;

    constexpr ImGuiTableFlags kFlags =
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoSavedSettings;

    if (!ImGui::BeginTable( "##propgrid", 2, kFlags ))
        return false;

    ImGui::TableSetupColumn( "Property", ImGuiTableColumnFlags_WidthFixed, kLabelWidth );
    ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_WidthStretch );

    PropertyGridWidgets widgets{ active_ctx_ ? active_ctx_->property_widgets : nullptr, active_ctx_ };
    bool changed = draw_record_fields( record, data, 0, active_ctx_ ? &widgets : nullptr );
    ImGui::EndTable();
    return changed;
}

} // namespace nc::editor
