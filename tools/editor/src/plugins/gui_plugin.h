#pragma once

#include <imgui.h>

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/services/video/window/window_types.h>

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
    HashMap<ImGuiMouseCursor, nc::CursorType> CursorMap;
    RID CanvasItem;
};

void register_gui_plugin( Scene& scene );
void unregister_gui_plugin( Scene& scene );

} // namespace nc::editor
