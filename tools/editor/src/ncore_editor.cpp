// clang-format off
#include <imgui_internal.h>
#include <ImGuizmo.h> // must come after imgui includes
// clang-format on

#include <filesystem>

#include <editor/ncore_editor.h>
#include <ncore/core/rid.h>
#include <ncore/game_world.h>
#include <ncore/runtime/components/material.h>
#include <ncore/runtime/components/services.h>
#include <ncore/runtime/components/time.h>
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/components/window.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/video/render_service.h>

#include "editor_camera.h"
#include "editor_state.h"
#include "gui_plugin.h"

namespace nc::editor {

static bool is_internal_component( const rtti::TypeInfo* type )
{
    return type == rtti::TypeRegistry::find<NodeRefComponent>();
}

static void draw_scene_tree_node( Node& node, EditorState& state )
{
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DrawLinesFull;
    if (node.get_child_count() == 0) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (node == state.SelectedNode) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    auto name = node.get_name();
    bool open = ImGui::TreeNodeEx( reinterpret_cast<void*>( node.get_id() ), flags, "%s", name.data() );

    if (ImGui::BeginDragDropSource( ImGuiDragDropFlags_None )) {
        Node* ptr = &node;
        ImGui::SetDragDropPayload( "SCENE_TREE_NODE", &ptr, sizeof( ptr ) );
        ImGui::Text( "%s", name.data() );
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "SCENE_TREE_NODE" )) {
            Node* dragged = *static_cast<Node**>( payload->Data );
            if (dragged && dragged != &node) {
                dragged->reparent_to( &node );
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::Button( "Rename" )) {
            auto n    = node.get_name();
            auto nlen = std::min( n.size(), sizeof( state.NodeRenameBuf ) - 1 );
            std::memcpy( state.NodeRenameBuf, n.data(), nlen );
            state.NodeRenameBuf[nlen] = '\0';
            state.NodeToRename        = &node;
            state.ShowRenamePopup     = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button( "Delete" )) {
            node.destroy();
            state.SelectedNode = nullptr; // avoids crashing the ECS
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        state.SelectedNode = &node;
    }

    if (open) {
        for (auto& child : node.get_children()) {
            draw_scene_tree_node( child, state );
        }
        ImGui::TreePop();
    }
}

#include "property_grid.h"

static bool draw_component_properties( const rtti::TypeInfo* type, void* data )
{
    return propgrid::draw_component_properties( type, data );
}

#include "ncore_editor_rest.inl"
