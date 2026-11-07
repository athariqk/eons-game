// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: vector_widgets.h - Editor UI widgets for vector types

#pragma once

#include <ncore/structs/color.h>

namespace sea::editor::widgets {

const nc::Color VECTOR_AXIS_COLORS[4] = {
    nc::Color( 235, 71, 71, 255 ),
    nc::Color( 82, 209, 82, 255 ),
    nc::Color( 82, 140, 242, 255 ),
    nc::Color( 217, 217, 89, 255 ),
};

struct DragComponentsDesc {
    int count;
    float speed;
    const char* const* formats = nullptr;
    const nc::Color* colors    = VECTOR_AXIS_COLORS;
    bool compact               = false;
};

bool drag_float_components( float* values, DragComponentsDesc p_desc );
bool drag_int_components( int* values, DragComponentsDesc p_desc );

} // namespace sea::editor::widgets
