#include <flecs.h>

#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/service_registry.h>

#include "scene_plugins.h"

namespace nc {

Scene::Scene( AppDesc& p_app_desc, ServiceRegistry& p_services ) : IGameWorld( p_app_desc, p_services ) {}

void Scene::on_enter()
{
    ecs_world.set_singleton<AppDesc>( app_desc );

    ecs_world.system( "Scene_NodeActivenessUpdater" )
        .with<NodeRefComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( QueryContext& ctx, EcsEntity id ) {
            auto ref   = ctx.get_component<NodeRefComponent>();
            auto world = reinterpret_cast<ecs_world_t*>( ctx.world().get_native_handle() );
            ecs_enable( world, id, ref->node->active );
        } );

    ecs_world.system( "Scene_ComponentActivenessUpdater" )
        .with<NodeRefComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( QueryContext& ctx, EcsEntity id ) {
            auto ref   = ctx.get_component<NodeRefComponent>();
            auto world = reinterpret_cast<ecs_world_t*>( ctx.world().get_native_handle() );
            for (auto& comp : ref->node->get_components()) {
                if (comp.CanToggleActive)
                    ecs_enable_id( world, id, comp.EcsId, comp.Active );
            }
        } );

    ecs_world.system( "Scene_Transform2DPropagator" )
        .in( EcsSystemPhase::UPDATE )
        .with<Transform2DComponent>()
        .each( []( QueryContext& ctx, EcsEntity eid ) {
            auto xform = ctx.get_component<Transform2DComponent>();

            NC_LOG_TRACE_C(
                log::ECS, "Entity: {}, Transform: {}", eid,
                rtti::TypeRegistry::to_string( xform, rtti::TypeRegistry::get_type_id<Transform2DComponent>() )
            );
        } );

    ecs_world.system( "Scene_Transform3DPropagator" )
        .in( EcsSystemPhase::UPDATE )
        .with<Transform3DComponent>()
        .each( []( QueryContext& ctx, EcsEntity ) {
            // auto xform = ctx.get_component<Transform3DComponent>();
        } );

    register_core_plugin( *this );
    register_window_plugin( *this );
    register_render_plugin( *this );
    register_inputs_plugin( *this );
    register_gui_plugin( *this );
    register_resources_plugin( *this );

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
    NC_LOG_TRACE_C( log::ECS, "Scene teardown" );
}

Node* Scene::root()
{
    ensure_root_node_exists_();
    return root_node;
}

void Scene::queue_destroy_node( Node* node )
{
    pending_node_deletions.push_back( node );
}

void Scene::ensure_root_node_exists_()
{
    if (root_node)
        return;

    root_node            = node_pool.acquire();
    root_node->scene     = this;
    root_node->node_pool = &node_pool;
    root_node->internal_id =
        ecs_world.entity( "RootNode" ).with<NodeRefComponent>( { root_node } ).with<RootNodeTag>().build();

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
