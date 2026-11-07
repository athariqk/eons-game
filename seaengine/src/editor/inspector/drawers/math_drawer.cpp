// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: math_drawer.cpp

#include "math_drawer.h"

#include <imgui.h>

#include <ncore/structs/quaternion.h>
#include <ncore/structs/rect.h>
#include <ncore/structs/vector.h>

#include "../widgets/vector_widgets.h"

namespace sea::editor {

bool draw_vec3f_property( PropertyDrawContext& ctx )
{
    nc::Vec3f* v = ctx.edit<nc::Vec3f>();
    if (!v)
        return false;
    constexpr const char* FMT[3] = { "X:%.3f", "Y:%.3f", "Z:%.3f" };
    return widgets::drag_float_components(
        v->data(), widgets::DragComponentsDesc{ .count = 3, .speed = 0.01f, .formats = FMT }
    );
}

bool draw_vec3i_property( PropertyDrawContext& ctx )
{
    // Vec3i may not be a distinct reflected type in all builds; fall back to raw pointer
    int* data = reinterpret_cast<int*>( ctx.editable.get_ptr() );
    if (!data)
        return false;
    constexpr const char* FMT[3] = { "X:%d", "Y:%d", "Z:%d" };
    return widgets::drag_int_components(
        data, widgets::DragComponentsDesc{ .count = 3, .speed = 1.0f, .formats = FMT }
    );
}

bool draw_quaternion_property( PropertyDrawContext& ctx )
{
    auto q       = ctx.edit<nc::Quaternion>();
    bool changed = false;

    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID mode_id = ImGui::GetID( "##quat_mode" );
    int mode              = storage->GetInt( mode_id, 0 ); // 0 = Euler, 1 = Raw

    {
        const char* labels[] = { "Euler", "Raw" };
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4.0f, 0.0f ) );
        for (int i = 0; i < 2; ++i) {
            if (i > 0)
                ImGui::SameLine();
            const bool selected = ( mode == i );
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
        if (ImGui::SmallButton( "I" )) {
            *q      = nc::Quaternion::identity();
            changed = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip( "Identity" );
        ImGui::SameLine();
        if (ImGui::SmallButton( "N" )) {
            *q      = nc::Quaternion::normalize( *q );
            changed = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip( "Normalize" );
        ImGui::PopStyleVar();
    }

    ImGui::SetNextItemWidth( -FLT_MIN );

    if (mode == 0) {
        nc::Vec3f euler_rad = nc::Quaternion::to_euler_angles( *q );
        float deg[3]        = {
            nc::math::rad_to_deg( euler_rad.x ),
            nc::math::rad_to_deg( euler_rad.y ),
            nc::math::rad_to_deg( euler_rad.z ),
        };
        constexpr const char* FMT[3] = { "X:%.3f", "Y:%.3f", "Z:%.3f" };
        if (widgets::drag_float_components(
                deg, widgets::DragComponentsDesc{ .count = 3, .speed = 0.5f, .formats = FMT }
            )) {
            *q = nc::Quaternion(
                nc::Vec3f(
                    nc::math::deg_to_rad( deg[0] ), nc::math::deg_to_rad( deg[1] ), nc::math::deg_to_rad( deg[2] )
                )
            );
            changed = true;
        }
    } else {
        float raw[4]                  = { q->w, q->v.x, q->v.y, q->v.z };
        const nc::Color RAW_COLORS[4] = {
            nc::Color( 191, 191, 191, 255 ),
            widgets::VECTOR_AXIS_COLORS[0],
            widgets::VECTOR_AXIS_COLORS[1],
            widgets::VECTOR_AXIS_COLORS[2],
        };
        static const char* FMT[4] = { "W:%.4f", "X:%.4f", "Y:%.4f", "Z:%.4f" };
        if (widgets::drag_float_components(
                raw, widgets::DragComponentsDesc{ .count = 4, .speed = 0.001f, .formats = FMT, .colors = RAW_COLORS }
            )) {
            q->w    = raw[0];
            q->v.x  = raw[1];
            q->v.y  = raw[2];
            q->v.z  = raw[3];
            changed = true;
        }
    }
    return changed;
}

bool draw_rect2f_property( PropertyDrawContext& ctx )
{
    auto r       = ctx.edit<nc::Rect2f>();
    bool changed = false;

    float pos[2] = { r->x, r->y };
    float sz[2]  = { r->w, r->h };

    ImGui::PushID( "pos" );
    ImGui::SetNextItemWidth( -FLT_MIN );
    constexpr const char* POS_FMT[2] = { "X:%.3f", "Y:%.3f" };
    if (widgets::drag_float_components(
            pos, widgets::DragComponentsDesc{ .count = 2, .speed = 0.01f, .formats = POS_FMT }
        )) {
        r->x    = pos[0];
        r->y    = pos[1];
        changed = true;
    }
    ImGui::PopID();

    static const nc::Color SIZE_COLORS[2] = {
        nc::Color( 217, 140, 38, 255 ),
        nc::Color( 140, 89, 217, 255 ),
    };
    constexpr const char* SIZE_FMT[2] = { "W:%.3f", "H:%.3f" };
    ImGui::PushID( "size" );
    if (widgets::drag_float_components(
            sz, widgets::DragComponentsDesc{ .count = 2, .speed = 0.01f, .formats = SIZE_FMT, .colors = SIZE_COLORS }
        )) {
        r->w    = sz[0];
        r->h    = sz[1];
        changed = true;
    }
    ImGui::PopID();

    return changed;
}

bool draw_rect2i_property( PropertyDrawContext& ctx )
{
    auto r       = ctx.edit<nc::Rect2i>();
    bool changed = false;

    int pos[2] = { r->x, r->y };
    int sz[2]  = { r->w, r->h };

    ImGui::PushID( "pos" );
    ImGui::SetNextItemWidth( -FLT_MIN );
    constexpr const char* POS_FMT[2] = { "X:%d", "Y:%d" };
    if (widgets::drag_int_components(
            pos, widgets::DragComponentsDesc{ .count = 2, .speed = 1.0f, .formats = POS_FMT }
        )) {
        r->x    = pos[0];
        r->y    = pos[1];
        changed = true;
    }
    ImGui::PopID();

    static const nc::Color SIZE_COLORS[2] = {
        nc::Color( 217, 140, 38, 255 ),
        nc::Color( 140, 89, 217, 255 ),
    };
    constexpr const char* SIZE_FMT[2] = { "W:%d", "H:%d" };
    ImGui::PushID( "size" );
    if (widgets::drag_int_components(
            sz, widgets::DragComponentsDesc{ .count = 2, .speed = 1.0f, .formats = SIZE_FMT, .colors = SIZE_COLORS }
        )) {
        r->w    = sz[0];
        r->h    = sz[1];
        changed = true;
    }
    ImGui::PopID();

    return changed;
}

bool draw_matrix3f_property( PropertyDrawContext& ctx )
{
    auto m       = ctx.edit<nc::Mat3>();
    bool changed = false;
    ImGui::PushID( "mat" );

    float* cols[3] = {};
    cols[0]        = m->col0.data();
    cols[1]        = m->col1.data();
    cols[2]        = m->col2.data();

    for (int row = 0; row < 3; ++row) {
        changed |= widgets::drag_float_components(
            cols[row], widgets::DragComponentsDesc{ .count = 3, .speed = 0.01f, .compact = true }
        );
    }
    ImGui::PopID();
    return changed;
}

bool draw_matrix4f_property( PropertyDrawContext& ctx )
{
    auto m       = ctx.edit<nc::Mat4>();
    bool changed = false;
    ImGui::PushID( "mat" );

    float* cols[4] = {};
    cols[0]        = m->col0.data();
    cols[1]        = m->col1.data();
    cols[2]        = m->col2.data();
    cols[3]        = m->col3.data();

    for (int row = 0; row < 4; ++row) {
        changed |= widgets::drag_float_components(
            cols[row], widgets::DragComponentsDesc{ .count = 4, .speed = 0.01f, .compact = true }
        );
    }
    ImGui::PopID();
    return changed;
}

} // namespace sea::editor
