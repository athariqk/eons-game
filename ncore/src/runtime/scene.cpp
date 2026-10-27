#include <format>

#include <flecs.h>

#include <ncore/application.h>
#include <ncore/runtime/components/config.h>
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <utils/profiler.h>

#include "scene_plugins.h"

namespace nc {

void Scene::on_enter()
{
    ecs_world.set_singleton<AppDesc>( app_ctx->app_desc );

    auto* config   = ecs_world.add_singleton<ConfigComponent>();
    config->config = &app_ctx->config;

    ecs_world.system( "Scene_NodeActivenessUpdater" )
        .with<NodeRefComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( EcsIterState& it ) {
            auto ref   = it.get_component<NodeRefComponent>();
            auto world = reinterpret_cast<ecs_world_t*>( it.world().get_native_handle() );
            ecs_enable( world, it.entity(), ref->node->active );
        } );

    ecs_world.system( "Scene_ComponentActivenessUpdater" )
        .with<NodeRefComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( EcsIterState& it ) {
            auto ref   = it.get_component<NodeRefComponent>();
            auto world = reinterpret_cast<ecs_world_t*>( it.world().get_native_handle() );
            for (auto& comp : ref->node->get_components()) {
                if (comp.Toggleable)
                    ecs_enable_id( world, it.entity(), comp.EcsId, comp.Active );
            }
        } );

    ecs_world.system( "Scene_Transform2D" )
        .in( EcsSystemPhase::UPDATE )
        .with<Transform2DComponent>()
        .each( []( EcsIterState& it ) {
            auto xform = it.get_component<Transform2DComponent>();
            ( void ) xform;
            // TODO: implement
        } );

    ecs_world.system( "Scene_Transform3D" )
        .in( EcsSystemPhase::UPDATE )
        .with<Transform3DComponent>()
        .with<Transform3DComponent>()
        .cascade()
        .without<Constraint3DComponent>()
        .each( []( EcsIterState& it ) {
            auto self   = it.get_component<Transform3DComponent>( 0 );
            auto parent = it.get_component<Transform3DComponent>( 1 );

            self->global = self->parent_contrib( parent->global ) * self->to_matrix();
        } );

    ecs_world.system( "Scene_Transform3D_Constrained" )
        .in( EcsSystemPhase::UPDATE )
        .with<Transform3DComponent, Constraint3DComponent>()
        .with<Transform3DComponent>()
        .cascade()
        .each( []( EcsIterState& it ) {
            auto self   = it.get_component<Transform3DComponent>( 0 );
            auto cons   = it.get_component<Constraint3DComponent>( 1 );
            auto parent = it.get_component<Transform3DComponent>( 2 );

            self->global = self->parent_contrib( parent->global ) * self->to_matrix();
            cons->apply_position_locks( self );
        } );

    register_core_plugin( *this );
    register_video_plugin( *this );
    register_inputs_plugin( *this );
    // register_gui_plugin( *this );
    register_resources_plugin( *this );
    register_debug_plugin( *this );
    ecs_world.finalize_ordering();

    ensure_root_node_exists_();
    on_ready();
    ecs_world.finalize_ordering();
}

void Scene::on_ready() {}

bool Scene::on_fixed_update( double p_delta )
{
    // TODO: run ecs in fixed-update pipeline
    return wants_to_quit;
}

bool Scene::on_variable_update( double p_delta )
{
    NC_ZONE_COLOR( "Scene::on_variable_update", NC_COLOR_ECS )
    process_pending_tasks_();
    process_pending_node_deletions_();
    ecs_world.progress( p_delta );
    return wants_to_quit;
}

void Scene::on_exit()
{
    NC_LOG_DEBUG_C( log::ECS, "Scene teardown" );
    // unregister_gui_plugin( *this );
}

Node* Scene::root()
{
    ensure_root_node_exists_();
    return root_node;
}

bool Scene::is_node_valid( Node* p_node )
{
    return p_node && node_pool.is_valid( p_node ) && !pending_node_deletions.contains( p_node );
}

Node* Scene::node_from_entity( EcsEntity p_entity )
{
    auto world = reinterpret_cast<ecs_world_t*>( ecs_world.get_native_handle() );
    if (p_entity == INVALID_ENTITY_ID || !ecs_is_alive( world, p_entity ))
        return nullptr;

    auto ref_id = ecs_world.resolve_component<NodeRefComponent>();
    auto ref    = static_cast<const NodeRefComponent*>( ecs_get_id( world, p_entity, ref_id ) );
    if (!ref || !ref->node)
        return nullptr;

    return node_pool.is_valid( ref->node ) ? ref->node : nullptr;
}

void Scene::queue_destroy_node( Node* p_node )
{
    pending_node_deletions.insert( p_node );
}

String Scene::generate_node_name()
{
    const auto formatted = std::format( "Node {}", ++node_name_counter );
    return String( formatted.c_str() );
}

void Scene::queue_task( std::function<void()> p_task )
{
    if (p_task)
        pending_tasks.push_back( std::move( p_task ) );
}

void Scene::ensure_root_node_exists_()
{
    if (root_node)
        return;

    root_node              = node_pool.acquire();
    root_node->scene       = this;
    root_node->node_pool   = &node_pool;
    root_node->internal_id = ecs_world.entity( "RootNode" )
                                 .add<NodeRefComponent>( NodeRefComponent{ .node = root_node } )
                                 .add<RootNodeTag>()
                                 .add<Transform3DComponent>()
                                 .build();

    NC_LOG_TRACE( "Root node was missing but is now created with entity ID {}", root_node->internal_id );
}

namespace {

// Children first: destroying a parent's entity takes its children with it.
void collect_subtree_post_order( Node& p_node, DynamicArray<Node*>& out, HashSet<Node*>& seen )
{
    for (auto& child : p_node.get_children()) {
        if (!seen.insert( &child ).second)
            continue;
        collect_subtree_post_order( child, out, seen );
    }
    out.push_back( &p_node );
}

} // namespace

void Scene::process_pending_tasks_()
{
    if (!pending_tasks.empty()) {
        auto tasks = std::move( pending_tasks );
        pending_tasks.clear();
        for (auto& task : tasks)
            task();
    }
}

void Scene::process_pending_node_deletions_()
{
    NC_ZONE_COLOR( "Scene::process_pending_node_deletions", NC_COLOR_ECS )

    if (pending_node_deletions.empty())
        return;

    DynamicArray<Node*> victims;
    HashSet<Node*> seen;
    for (auto* node : pending_node_deletions) {
        if (!node_pool.is_valid( node ) || !seen.insert( node ).second)
            continue;
        collect_subtree_post_order( *node, victims, seen );
    }
    pending_node_deletions.clear();

    for (auto* node : victims) {
        NC_LOG_DEBUG_C( log::ECS, "Destroying node: Name={} ID={}", node->get_name(), node->internal_id );
        // safe when the node never built a child query: remove_query() no-ops.
        ecs_world.remove_query( node->child_query.get_name() );
        if (ecs_world.is_alive( node->get_id() ))
            ecs_world.destroy_entity( node->get_id() );
        node_pool.release( node );
    }
}

} // namespace nc
