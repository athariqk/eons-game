#include "scene_tree_panel.h"

#include <algorithm>
#include <filesystem>

#include <ncore/runtime/components.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/structs/vector.h>

#include <editor/editor_components.h>
#include <editor/io/editor_assets.h>

namespace sea::editor {

namespace {

// Spawn a node rendering a mesh asset (drag-drop from the asset list). Queued
// through Scene: entity creation cannot happen while the ECS runs. The mesh is
// resolved through EditorAssets on the main thread and captured by the task.
bool spawn_mesh_from_asset( EditorAssets& assets, nc::Scene& scene, nc::Node& parent, nc::StringView path )
{
    const nc::EcsEntity parent_id = parent.get_id();
    const nc::String asset_path( path.data(), path.size() );

    nc::Ref<nc::Mesh> mesh = assets.load<nc::Mesh>( asset_path );
    if (!mesh)
        return false;

    scene.queue_task( [&scene, parent_id, asset_path, mesh_handle = std::move( mesh )]() {
        auto* parent_node = scene.get_node_from_entity( parent_id );
        if (!parent_node)
            return;

        const std::string stem = std::filesystem::path( asset_path.c_str() ).stem().string();

        // The mesh resource owns its geometry and material; the entity only
        // stores the reference (resolved lazily by the render systems).
        auto node = parent_node->create_child( nc::String( stem.c_str() ) );
        node->add_component<nc::Transform3DComponent>();
        node->add_component<nc::MeshInstanceComponent>( nc::MeshInstanceComponent{ .mesh = mesh_handle } );
    } );

    return true;
}

} // namespace

SceneTreePanel::SceneTreePanel( nc::Scene& p_scene, EditorSelection& p_selection, EditorAssets& p_assets ) :
    EditorPanel( "Scene Tree", DockSlot::LEFT_UPPER ), scene( p_scene ), selection( p_selection ), assets( p_assets )
{}

void SceneTreePanel::draw_contents( EditorFrame& frame )
{
    nc::Node* root = scene.root();

    if (root) {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        draw_node( *root, frame );
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "SCENE_TREE_NODE" )) {
            const nc::EcsEntity dragged_id = *static_cast<const nc::EcsEntity*>( payload->Data );
            nc::Node* dragged              = scene.get_node_from_entity( dragged_id );
            if (dragged && root && dragged != root)
                dragged->reparent_to( root );
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "ASSET_PATH" )) {
            const AssetEntry* entry = assets.find( static_cast<const char*>( payload->Data ) );
            if (entry && entry->kind == AssetKind::MESH && root)
                spawn_mesh_from_asset( assets, scene, *root, nc::StringView( entry->path ) );
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextWindow( "Scene Tree Context Menu", ImGuiPopupFlags_NoOpenOverItems )) {
        if (ImGui::Button( "Spawn Entity" ) && root) {
            const nc::EcsEntity root_id = root->get_id();
            scene.queue_task( [&]() {
                if (auto* root_node = scene.get_node_from_entity( root_id ))
                    root_node->create_child();
            } );
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void SceneTreePanel::post_end( EditorFrame& frame )
{
    draw_rename_modal( frame );
}

void SceneTreePanel::queue_destroy_subtree( nc::Node& node )
{
    for (auto& child : node.get_children())
        queue_destroy_subtree( child );
    node.destroy();
}

void SceneTreePanel::draw_node( nc::Node& node, EditorFrame& frame )
{
    if (is_editor_internal_node( node ))
        return;

    const bool is_root = node.has_component<nc::RootNodeTag>();

    ImGui::PushID( static_cast<int>( node.get_id() ) );

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DrawLinesFull;
    if (node.get_child_count() == 0)
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (node.get_id() == selection.get_selected_eid())
        flags |= ImGuiTreeNodeFlags_Selected;

    const auto name = node.get_name();
    const bool open = ImGui::TreeNodeEx( reinterpret_cast<void*>( node.get_id() ), flags, "%s", name.data() );

    // EditorSelection and the context menu must be resolved right after the item;
    // drag-drop below submits other widgets and would steal "last item".
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        selection.select( node.get_id() );

    if (!is_root)
        ImGui::OpenPopupOnItemClick( "##node_menu", ImGuiPopupFlags_MouseButtonRight );

    if (!is_root && ImGui::BeginPopup( "##node_menu" )) {
        if (ImGui::MenuItem( "Spawn Child" )) {
            const nc::EcsEntity parent = node.get_id();
            scene.queue_task( [&]() {
                if (auto* parent_node = scene.get_node_from_entity( parent ))
                    parent_node->create_child();
            } );
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem( "Rename" )) {
            const auto current = node.get_name();
            const auto len     = std::min( current.size(), sizeof( rename_buf ) - 1 );
            std::memcpy( rename_buf, current.data(), len );
            rename_buf[len] = '\0';
            rename_entity   = node.get_id();
            show_rename     = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::Separator();

        if (ImGui::MenuItem( "Delete" )) {
            nc::Node* selected = selection.node();
            if (selected && ( selected == &node || selected->is_descendant_of( node ) ))
                selection.clear();

            queue_destroy_subtree( node );
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (!is_root && ImGui::BeginDragDropSource( ImGuiDragDropFlags_None )) {
        // Entity ID, not Node*: pooled node pointers are recycled across frames.
        const nc::EcsEntity dragged_id = node.get_id();
        ImGui::SetDragDropPayload( "SCENE_TREE_NODE", &dragged_id, sizeof( dragged_id ) );
        ImGui::Text( "%s", name.data() );
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "SCENE_TREE_NODE" )) {
            const nc::EcsEntity dragged_id = *static_cast<const nc::EcsEntity*>( payload->Data );
            nc::Node* dragged              = scene.get_node_from_entity( dragged_id );
            if (dragged && dragged != &node)
                dragged->reparent_to( &node );
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "ASSET_PATH" )) {
            const AssetEntry* entry = assets.find( static_cast<const char*>( payload->Data ) );
            if (entry && entry->kind == AssetKind::MESH)
                spawn_mesh_from_asset( assets, scene, node, nc::StringView( entry->path ) );
        }
        ImGui::EndDragDropTarget();
    }

    if (open) {
        // Flecs stores entities per archetype, so adding or removing anything
        // (selection is a tag; the inspector adds components) moves the row in
        // child-query order — the node appears to jump to the bottom. Sort by
        // entity id, which is creation order and never changes, so the authored
        // hierarchy stays put.
        nc::DynamicArray<nc::Node*> children;
        children.reserve( node.get_child_count() );
        for (auto& child : node.get_children())
            children.push_back( &child );

        std::sort( children.begin(), children.end(), []( const nc::Node* a, const nc::Node* b ) {
            return a->get_id() < b->get_id();
        } );

        for (auto* child : children)
            draw_node( *child, frame );

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void SceneTreePanel::draw_rename_modal( EditorFrame& )
{
    if (show_rename) {
        ImGui::OpenPopup( "Entity Rename" );
        show_rename = false;
    }

    if (!ImGui::BeginPopupModal( "Entity Rename", nullptr, ImGuiWindowFlags_AlwaysAutoResize ))
        return;

    ImGui::InputText(
        "##rename", rename_buf, sizeof( rename_buf ),
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll
    );

    const bool confirmed = ImGui::Button( "OK" ) || ImGui::IsKeyPressed( ImGuiKey_Enter );
    if (confirmed && rename_buf[0]) {
        nc::Node* to_rename = scene.get_node_from_entity( rename_entity );
        if (to_rename)
            to_rename->set_name( rename_buf );

        rename_entity = nc::INVALID_ENTITY_ID;
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button( "Cancel" )) {
        rename_entity = nc::INVALID_ENTITY_ID;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

} // namespace sea::editor
