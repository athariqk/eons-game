// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: editor_panel.cpp

#include "editor_panel.h"

namespace sea::editor {

void EditorPanel::draw_window( EditorFrame& frame )
{
    pre_begin( frame );

    if (is_visible) {
        bool drawn = ImGui::Begin( title.c_str(), is_closable ? &is_visible : nullptr, window_flags() );
        if (drawn) {
            draw_contents( frame );
        } else {
            on_collapsed( frame );
        }
        ImGui::End();
    }

    post_end( frame );
}

} // namespace sea::editor
