#include <flecs.h>

#include <ncore/application.h>
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>

#include "scene_plugins.h"

namespace nc {

void Scene::on_enter()
{
    ecs_world.set_singleton<AppDesc>( app_ctx->AppDesc );

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
        .up()
        .each( []( EcsIterState& it ) {
            auto self    = it.get_component<Transform3DComponent>( 0 );
            auto parent  = it.get_component<Transform3DComponent>( 1 );
            self->Global = parent->Global * self->to_matrix();
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

bool Scene::is_node_valid( Node* node )
{
    return node && node_pool.is_valid( node ) && !pending_node_deletions.contains( node );
}

void Scene::queue_destroy_node( Node* node )
{
    pending_node_deletions.insert( node );
}

void Scene::ensure_root_node_exists_()
{
    if (root_node)
        return;

    root_node              = node_pool.acquire();
    root_node->scene       = this;
    root_node->node_pool   = &node_pool;
    root_node->internal_id = ecs_world.entity( "RootNode" )
                                 .add<NodeRefComponent>( { root_node } )
                                 .add<RootNodeTag>()
                                 .add<Transform3DComponent>()
                                 .build();

    NC_LOG_TRACE( "Root node was missing but is now created with entity ID {}", root_node->internal_id );
}

void Scene::process_pending_node_deletions_()
{
    for (auto& node : pending_node_deletions) {
        NC_LOG_DEBUG_C( log::ECS, "Destroying node: Name={} ID={}", node->get_name(), node->internal_id );
        ecs_world.remove_query( node->child_query.get_name() );
        ecs_world.destroy_entity( node->get_id() );
        node_pool.release( node );
    }
    pending_node_deletions.clear();
}

} // namespace nc
