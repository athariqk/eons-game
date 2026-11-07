// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: resource_drawer.cpp

#include "resource_drawer.h"

#include <imgui.h>

#include <ncore/structs/rid.h>

namespace sea::editor {

bool draw_rid_property( PropertyDrawContext& ctx )
{
    auto rid = ctx.edit<nc::RID>();
    if (!rid->is_valid()) {
        ImGui::TextDisabled( "Invalid" );
        return false;
    }

    const uint32_t index = static_cast<uint32_t>( rid->value & 0xFFFFFFFFu );
    const uint32_t gen   = static_cast<uint32_t>( rid->value >> 32 );

    ImGui::Text( "%u:%u", index, gen );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( -FLT_MIN );
    return ImGui::InputScalar(
        "##rid", ImGuiDataType_U64, &rid->value, nullptr, nullptr, "0x%llX", ImGuiInputTextFlags_CharsHexadecimal
    );
}

} // namespace sea::editor
