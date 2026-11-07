// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: editor_bootstrap.h

#pragma once

namespace nc {
class Scene;
}

namespace sea::editor {

void bootstrap_editor( nc::Scene& p_scene );

/**
 * @brief Symmetric shutdown for the editor singleton + GUI plugin.
 * Must run before renderer and scene services are destroyed.
 */
void shutdown_editor( nc::Scene& p_scene );

} // namespace sea::editor
