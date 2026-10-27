#include <algorithm>
#include <editor_components.h>

#include <ncore/runtime/scene.h>

#include <io/editor_assets.h>
#include <panels/assets_panel.h>
#include <panels/scene_tree_panel.h>

namespace nc::editor {

SceneTreePanel::SceneTreePanel() : EditorPanel( "Scene Tree", DockSlot::LeftUpper ) {}

void SceneTreePanel::draw_node( Node& node, EditorContext& ctx )
{
    if (is_editor_internal_node( node ))
        return;

    const bool is_root = node.has_component<RootNodeTag>();

    ImGui::PushID( static_cast<int>( node.get_id() ) );

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DrawLinesFull;
    if (node.get_child_count() == 0)
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (node.get_id() == ctx.selected_eid)
        flags |= ImGuiTreeNodeFlags_Selected;

    const auto name = node.get_name();
    const bool open = ImGui::TreeNodeEx( reinterpret_cast<void*>( node.get_id() ), flags, "%s", name.data() );

    // Selection and the context menu must be resolved right after the item;
    // drag-drop below submits other widgets and would steal "last item".
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && ctx.selection_state)
        ctx.selection_state->select( *ctx.ecs_world, node.get_id() );

    if (!is_root)
        ImGui::OpenPopupOnItemClick( "##node_menu", ImGuiPopupFlags_MouseButtonRight );

    if (!is_root && ImGui::BeginPopup( "##node_menu" )) {
        if (ImGui::MenuItem( "Spawn Child" )) {
            if (Scene* scene = ctx.scene) {
                const EcsEntity parent = node.get_id();
                scene->queue_task( [scene, parent]() {
                    if (auto* parent_node = scene->node_from_entity( parent ))
                        parent_node->create_child();
                } );
            }
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem( "Rename" )) {
            const auto current = node.get_name();
            const auto len     = std::min( current.size(), sizeof( rename_buf_ ) - 1 );
            std::memcpy( rename_buf_, current.data(), len );
            rename_buf_[len] = '\0';
            rename_entity_   = node.get_id();
            show_rename_     = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::Separator();

        if (ImGui::MenuItem( "Delete" )) {
            Node* selected = ctx.selection_state ? ctx.selection_state->node( *ctx.scene ) : nullptr;
            if (selected && ( selected == &node || selected->is_descendant_of( node ) ) && ctx.scene)
                ctx.selection_state->clear( *ctx.ecs_world );

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
            Node* dragged              = ctx.scene ? ctx.scene->node_from_entity( dragged_id ) : nullptr;
            if (dragged && dragged != &node)
                dragged->reparent_to( &node );
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "ASSET_PATH" )) {
            const AssetEntry* entry =
                ctx.assets ? ctx.assets->find( static_cast<const char*>( payload->Data ) ) : nullptr;
            if (entry && entry->kind == AssetKind::MESH && ctx.loader)
                spawn_mesh_from_asset( *ctx.scene, node, StringView( entry->path ), *ctx.loader );
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

void SceneTreePanel::draw_rename_modal( EditorContext& ctx )
{
    if (show_rename_) {
        ImGui::OpenPopup( "Entity Rename" );
        show_rename_ = false;
    }

    if (!ImGui::BeginPopupModal( "Entity Rename", nullptr, ImGuiWindowFlags_AlwaysAutoResize ))
        return;

    ImGui::InputText(
        "##rename", rename_buf_, sizeof( rename_buf_ ),
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll
    );

    const bool confirmed = ImGui::Button( "OK" ) || ImGui::IsKeyPressed( ImGuiKey_Enter );
    if (confirmed && rename_buf_[0] && ctx.scene) {
        Node* to_rename = ctx.scene->node_from_entity( rename_entity_ );
        if (to_rename)
            to_rename->set_name( rename_buf_ );

        rename_entity_ = INVALID_ENTITY_ID;
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button( "Cancel" )) {
        rename_entity_ = INVALID_ENTITY_ID;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void SceneTreePanel::draw( EditorContext& ctx )
{
    if (!ctx.scene)
        return;

    Node* root = ctx.scene->root();

    if (!ImGui::Begin( title.c_str(), &is_visible )) {
        ImGui::End();
        return;
    }

    if (root) {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        draw_node( *root, ctx );
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "SCENE_TREE_NODE" )) {
            const EcsEntity dragged_id = *static_cast<const EcsEntity*>( payload->Data );
            Node* dragged              = ctx.scene->node_from_entity( dragged_id );
            if (dragged && root && dragged != root)
                dragged->reparent_to( root );
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "ASSET_PATH" )) {
            const AssetEntry* entry =
                ctx.assets ? ctx.assets->find( static_cast<const char*>( payload->Data ) ) : nullptr;
            if (entry && entry->kind == AssetKind::MESH && root && ctx.loader)
                spawn_mesh_from_asset( *ctx.scene, *root, StringView( entry->path ), *ctx.loader );
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextWindow( "Scene Tree Context Menu", ImGuiPopupFlags_NoOpenOverItems )) {
        if (ImGui::Button( "Spawn Entity" ) && root) {
            Scene* scene            = ctx.scene;
            const EcsEntity root_id = root->get_id();
            scene->queue_task( [scene, root_id]() {
                if (auto* root_node = scene->node_from_entity( root_id ))
                    root_node->create_child();
            } );
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();

    draw_rename_modal( ctx );
}

} // namespace nc::editor
