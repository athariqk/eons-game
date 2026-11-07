#pragma once

#include <ncore/core/memory.h>

namespace nc {
class RenderService;
class Scene;
} // namespace nc

namespace sea::editor {

class Editor;

/**
 * @brief Composition root: resolve app services once and assemble the editor.
 *
 * This is the only editor-side place that knows about AppContext / ServiceDB.
 * Returns null when the required renderer is unavailable.
 */
nc::Ptr<Editor> create_editor( nc::Scene& p_scene );

} // namespace sea::editor
