#include <algorithm>
#include <editor_components.h>
#include <filesystem>

#include <ncore/runtime/components.h>
#include <ncore/runtime/io/resource_loader.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/structs/vector.h>

#include <io/editor_assets.h>
#include <panels/scene_tree_panel.h>

namespace nc::editor {

namespace {

// Spawn a node rendering a mesh asset (drag-drop from the asset list). Queued
// through Scene: entity creation cannot happen while the ECS runs.
bool spawn_mesh_from_asset( Scene& scene, Node& parent, StringView path )
{
    const EcsEntity parent_id = parent.get_id();
    const String asset_path( path.data(), path.size() );

    scene.queue_task( [&scene, parent_id, asset_path]() {
        auto* parent_node = scene.get_node_from_entity( parent_id );
        if (!parent_node)
            return;

        const std::string stem = std::filesystem::path( asset_path.c_str() ).stem().string();

        // The mesh resource owns its geometry and material; the entity only
        // stores the reference (resolved lazily by the render systems).
        auto node = parent_node->create_child( String( stem.c_str() ) );
        node->add_component<Transform3DComponent>();
        node->add_component<MeshInstanceComponent>(
            MeshInstanceComponent{ .mesh = scene.get_resource_loader().load<Mesh>( asset_path ) }
        );
    } );

    return true;
}

} // namespace

SceneTreePanel::SceneTreePanel( Scene& p_scene, EditorSelection& p_selection, AssetDatabase& p_assets ) :
    EditorPanel( "Scene Tree", DockSlot::LeftUpper ), scene( p_scene ), selection( p_selection ), assets( p_assets )
{}

void SceneTreePanel::draw_contents( EditorFrame& ctx )
{
    Node* root = scene.root();

    if (root) {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        draw_node( *root, ctx );
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "SCENE_TREE_NODE" )) {
            const EcsEntity dragged_id = *static_cast<const EcsEntity*>( payload->Data );
            Node* dragged              = scene.get_node_from_entity( dragged_id );
            if (dragged && root && dragged != root)
                dragged->reparent_to( root );
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "ASSET_PATH" )) {
            const AssetEntry* entry = assets.find( static_cast<const char*>( payload->Data ) );
            if (entry && entry->kind == AssetKind::MESH && root)
                spawn_mesh_from_asset( scene, *root, StringView( entry->path ) );
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextWindow( "Scene Tree Context Menu", ImGuiPopupFlags_NoOpenOverItems )) {
        if (ImGui::Button( "Spawn Entity" ) && root) {
            const EcsEntity root_id = root->get_id();
            scene.queue_task( [&]() {
                if (auto* root_node = scene.get_node_from_entity( root_id ))
                    root_node->create_child();
            } );
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void SceneTreePanel::post_end( EditorFrame& ctx )
{
    draw_rename_modal( ctx );
}

void SceneTreePanel::queue_destroy_subtree( Node& node )
{
    for (auto& child : node.get_children())
        queue_destroy_subtree( child );
    node.destroy();
}

void SceneTreePanel::draw_node( Node& node, EditorFrame& ctx )
{
    if (is_editor_internal_node( node ))
        return;

    const bool is_root = node.has_component<RootNodeTag>();

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
        selection.select( ctx.scene->get_ecs(), node.get_id() );

    if (!is_root)
        ImGui::OpenPopupOnItemClick( "##node_menu", ImGuiPopupFlags_MouseButtonRight );

    if (!is_root && ImGui::BeginPopup( "##node_menu" )) {
        if (ImGui::MenuItem( "Spawn Child" )) {
            const EcsEntity parent = node.get_id();
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
            Node* selected = selection.node( scene );
            if (selected && ( selected == &node || selected->is_descendant_of( node ) ))
                selection.clear( ctx.scene->get_ecs() );

            queue_destroy_subtree( node );
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (!is_root && ImGui::BeginDragDropSource( ImGuiDragDropFlags_None )) {
        // Entity ID, not Node*: pooled node pointers are recycled across frames.
        const EcsEntity dragged_id = node.get_id();
        ImGui::SetDragDropPayload( "SCENE_TREE_NODE", &dragged_id, sizeof( dragged_id ) );
        ImGui::Text( "%s", name.data() );
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "SCENE_TREE_NODE" )) {
            const EcsEntity dragged_id = *static_cast<const EcsEntity*>( payload->Data );
            Node* dragged              = scene.get_node_from_entity( dragged_id );
            if (dragged && dragged != &node)
                dragged->reparent_to( &node );
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "ASSET_PATH" )) {
            const AssetEntry* entry = assets.find( static_cast<const char*>( payload->Data ) );
            if (entry && entry->kind == AssetKind::MESH)
                spawn_mesh_from_asset( scene, node, StringView( entry->path ) );
        }
        ImGui::EndDragDropTarget();
    }

    if (open) {
        // Flecs stores entities per archetype, so adding or removing anything
        // (selection is a tag; the inspector adds components) moves the row in
        // child-query order — the node appears to jump to the bottom. Sort by
        // entity id, which is creation order and never changes, so the authored
        // hierarchy stays put.
        DynamicArray<Node*> children;
        children.reserve( node.get_child_count() );
        for (auto& child : node.get_children())
            children.push_back( &child );

        std::sort( children.begin(), children.end(), []( const Node* a, const Node* b ) {
            return a->get_id() < b->get_id();
        } );

        for (auto* child : children)
            draw_node( *child, ctx );

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void SceneTreePanel::draw_rename_modal( EditorFrame& ctx )
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
        Node* to_rename = scene.get_node_from_entity( rename_entity );
        if (to_rename)
            to_rename->set_name( rename_buf );

        rename_entity = INVALID_ENTITY_ID;
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button( "Cancel" )) {
        rename_entity = INVALID_ENTITY_ID;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

} // namespace nc::editor
