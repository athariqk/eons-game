#pragma once

#include <ncore/runtime/ecs/ecs_component.h>

namespace nc::editor {

/**
 * @brief Marks the editor's own free-fly camera (never part of the authored scene).
 */
NC_COMPONENT( EditorCameraTag ){};

/**
 * @brief ECS marker for the selected entity (readable by engine-side systems).
 */
NC_COMPONENT( SelectedTag ){};

} // namespace nc::editor
