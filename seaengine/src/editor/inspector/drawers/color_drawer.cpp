// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: color_drawer.cpp

#include "color_drawer.h"

#include <imgui.h>

#include <ncore/structs/color.h>

namespace sea::editor {

bool draw_color_property( PropertyDrawContext& ctx )
{
    auto c        = ctx.edit<nc::Color>();
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

} // namespace sea::editor
