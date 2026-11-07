// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: editor_frame.h

#pragma once

namespace nc {
class Scene;
}

namespace sea::editor {

/**
 * @brief Per-frame state a panel reads while drawing.
 *
 * Stable dependencies (services, assets, selection, settings) belong in a
 * panel's constructor. Anything that changes every frame lives here and only
 * here: no services, no registries, no ownership.
 */
struct EditorFrame {
    nc::Scene* scene = nullptr;

    // Published by the viewport panels while drawing.
    bool any_viewport_hovered = false;
    bool any_viewport_focused = false;
};

} // namespace sea::editor
