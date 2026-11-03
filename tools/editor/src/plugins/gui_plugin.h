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

namespace nc::editor {

/**
 * @brief ImGui context / canvas resources owned by the GUI plugin.
 */
NC_COMPONENT( GuiStateComponent )
{
    ImGuiContext* imgui_ctx = nullptr;
    HashMap<ImGuiMouseCursor, nc::CursorType> cursor_map;
    RID canvas_item;
    /// Keeps the canvas material resource (and its GPU instance) alive.
    Ref<Material> canvas_material;
};

void register_gui_plugin( Scene& scene );
void unregister_gui_plugin( Scene& scene );

} // namespace nc::editor
