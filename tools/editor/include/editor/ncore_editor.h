#pragma once

#include <ncore.hpp>

// Export annotation for ncore_editor's own entry points.
// NCAPI tracks ncore_EXPORTS and is therefore always dllimport here.
#ifdef ncore_editor_EXPORTS
#define NCAPI_EDITOR __declspec( dllexport )
#elif defined( WIN32 )
#define NCAPI_EDITOR __declspec( dllimport )
#else
#define NCAPI_EDITOR
#endif

namespace nc {
class Scene;
}

namespace nc::editor {

/**
 * @brief Should be called before any other ECS calls.
 */
void NCAPI_EDITOR register_editor_plugin( Scene& scene );
/**
 * @brief Should be called before any other plugin unregistrations.
 */
void NCAPI_EDITOR unregister_editor_plugin( Scene& scene );

} // namespace nc::editor
