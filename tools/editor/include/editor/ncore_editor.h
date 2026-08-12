#pragma once

#include <ncore.hpp>

namespace nc {
class Scene;
}

namespace nc::editor {

/**
 * @brief Should be registered before any other ECS calls.
 */
void NCAPI register_editor_plugin( Scene& scene );

} // namespace nc::editor
