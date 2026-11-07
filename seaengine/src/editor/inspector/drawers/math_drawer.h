// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: math_drawer.h - Property drawers for math types

#pragma once

#include <editor/inspector/property_draw_context.h>

namespace sea::editor {

bool draw_vec3f_property( PropertyDrawContext& ctx );
bool draw_vec3i_property( PropertyDrawContext& ctx );
bool draw_quaternion_property( PropertyDrawContext& ctx );
bool draw_rect2f_property( PropertyDrawContext& ctx );
bool draw_rect2i_property( PropertyDrawContext& ctx );
bool draw_matrix3f_property( PropertyDrawContext& ctx );
bool draw_matrix4f_property( PropertyDrawContext& ctx );

} // namespace sea::editor
