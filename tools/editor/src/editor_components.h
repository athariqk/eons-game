#pragma once

#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>

#include "editor.h"

namespace nc::editor {

NC_COMPONENT( EditorPlugin )
{
    Ptr<Editor> editor;
};

/**
 * @brief Marks the editor's own free-fly camera (never part of the authored scene).
 */
NC_COMPONENT( EditorCameraTag ){};

/**
 * @brief ECS marker for the selected entity (readable by engine-side systems).
 */
NC_COMPONENT( SelectedTag ){};

/**
 * @brief Register editor-owned component types with the world.
 *
 * Flecs refuses to apply traits (EcsCanToggle) to a component once a query
 * references it, so this must run before any editor query or system exists.
 */
void register_editor_component_types( Scene& scene );

bool is_internal_component( const types::TypeInfo* type );

bool is_editor_internal_node( Node& node );

} // namespace nc::editor
