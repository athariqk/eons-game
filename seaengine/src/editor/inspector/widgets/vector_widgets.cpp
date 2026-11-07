// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: vector_widgets.cpp

#include "vector_widgets.h"

#include <imgui.h>

namespace sea::editor::widgets {

template<class T>
bool drag_axis( T* values, DragComponentsDesc p_desc )
{
    bool changed        = false;
    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    const float total   = ImGui::CalcItemWidth();
    const float cell =
        ( total - spacing * static_cast<float>( p_desc.count - 1 ) ) / static_cast<float>( p_desc.count );
    for (int i = 0; i < p_desc.count; ++i) {
        if (i > 0)
            ImGui::SameLine( 0.0f, spacing );
        ImGui::PushID( i );
        bool pushed_color = false;
        if (p_desc.colors) {
            const nc::Vec4f axis = p_desc.colors[i].normalize();
            ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( axis.x, axis.y, axis.z, axis.w ) );
            pushed_color = true;
        }
        ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 1.5f );
        ImGui::SetNextItemWidth( p_desc.compact ? -FLT_MIN : cell );
        const char* format = p_desc.formats ? p_desc.formats[i] : nullptr;
        if (!format)
            format = std::is_same_v<T, float> ? "%.3f" : "%d";
        if constexpr (std::is_same_v<T, float>)
            changed |= ImGui::DragFloat( "##c", &values[i], p_desc.speed, 0.0f, 0.0f, format );
        else
            changed |= ImGui::DragInt( "##c", &values[i], p_desc.speed, 0, 0, format );
        ImGui::PopStyleVar();
        if (pushed_color)
            ImGui::PopStyleColor();
        ImGui::PopID();
    }
    return changed;
}

bool drag_float_components( float* values, DragComponentsDesc p_desc )
{
    return drag_axis<float>( values, p_desc );
}

bool drag_int_components( int* values, DragComponentsDesc p_desc )
{
    return drag_axis<int>( values, p_desc );
}

} // namespace sea::editor::widgets
