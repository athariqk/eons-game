#pragma once

#include <ncore.hpp>

namespace nc {
class Scene;
}

namespace nc::editor {

/**
 * @brief Should be called before any other ECS calls.
 */
void NCAPI register_editor_plugin( Scene& scene );
/**
 * @brief Should be called before any other plugin unregistrations.
 */
void NCAPI unregister_editor_plugin( Scene& scene );

} // namespace nc::editor
