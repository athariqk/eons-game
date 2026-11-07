#pragma once

#include <imgui.h>

#include <ncore/core/collection.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/runtime/resources/material.h>
#include <ncore/services/video/window/window_types.h>
#include <ncore/structs/rid.h>

namespace nc {
class Scene;
}

namespace sea::editor {

/**
 * @brief ImGui context / canvas resources owned by the GUI plugin.
 */
NC_COMPONENT( EditorGuiComponent )
{
    ImGuiContext* imgui_ctx = nullptr;
    nc::HashMap<ImGuiMouseCursor, nc::CursorType> cursor_map;
    nc::RID canvas_item;
    /// Keeps the canvas material resource (and its GPU instance) alive.
    nc::Ref<nc::Material> canvas_material;
};

void register_gui_plugin( nc::Scene& scene );
void unregister_gui_plugin( nc::Scene& scene );

} // namespace sea::editor
