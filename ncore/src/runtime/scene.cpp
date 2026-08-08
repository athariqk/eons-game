#include <flecs.h>

#include <ncore/runtime/components/resource.h>
#include <ncore/runtime/components/services.h>
#include <ncore/runtime/components/time.h>
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/input/input_service.h>
#include <ncore/services/io/resource_service.h>
#include <ncore/services/service_registry.h>
#include <ncore/services/video/render_service.h>
#include <ncore/services/video/window_service.h>

#include "scene_plugins.h"

namespace nc {

struct ResourceWatchState {
    DynamicArray<ResourceService::Event> pending_events;
    NSTRUCT( ResourceWatchState, NC_F( ResourceWatchState, pending_events ) )
};

Scene::Scene( AppDesc& p_app_desc, ServiceRegistry& p_services ) : IGameWorld( p_app_desc, p_services ) {}

void Scene::on_enter()
{
    ecs_world.set_singleton<AppDesc>( app_desc );
    ecs_world.emplace_singleton<TimeComponent>();
    ecs_world.emplace_singleton<IoServices>();
    ecs_world.emplace_singleton<GraphicsServices>();
    ecs_world.emplace_singleton<ResourceWatchState>();

    add_startup( []( Node& root ) {
        auto& svcs = root.get_scene()->get_services();
        auto io    = root.get_scene()->get_ecs().get_singleton<IoServices>();
        auto gfx   = root.get_scene()->get_ecs().get_singleton<GraphicsServices>();

        io->resources = svcs.resolve<ResourceService>();
        io->inputs    = svcs.resolve<InputService>();
        gfx->window   = svcs.resolve<WindowService>();
        gfx->renderer = svcs.resolve<RenderService>();
    } );

    ecs_world.system( "Scene_FPSTracker" )
        .with<TimeComponent>()
        .in( EcsSystemPhase::PRE_FRAME )
        .run( []( QueryContext& ctx ) {
            auto time = ctx.get_component<TimeComponent>();
            time->ticks++;
            time->frame_count++;
            time->accumulator += ctx.delta_time();
            if (time->accumulator >= 1.0) {
                time->fps         = static_cast<double>( time->frame_count ) / time->accumulator;
                time->frame_count = 0;
                time->accumulator = 0.0;
            }
        } );

    register_window_plugin( *this );
    register_render_plugin( *this );
    register_inputs_plugin( *this );
    register_gui_plugin( *this );

    ecs_world.system( "Scene_ResourceWatcher_Poll" ).in( EcsSystemPhase::POST_UPDATE ).run( []( QueryContext& ctx ) {
        auto io    = ctx.world().get_singleton<IoServices>();
        auto state = ctx.world().get_singleton<ResourceWatchState>();

        state->pending_events.clear();
        ResourceService::Event e;
        while (io->resources->poll_event( &e )) {
            state->pending_events.push_back( e );
        }
    } );

    ecs_world.system( "Scene_ResourceWatcher_Emit" )
        .with<HasResourceTag>()
        .in( EcsSystemPhase::POST_UPDATE )
        .each( []( QueryContext& ctx, EcsEntity id ) {
            auto state = ctx.world().get_singleton<ResourceWatchState>();
            for (auto& entry : state->pending_events) {
                if (auto loaded = std::get_if<ResourceService::LoadEvent>( &entry )) {
                    NC_LOG_DEBUG(
                        "ResourceService::LoadEvent: RID={} ResourceFormatID={}", loaded->handle.value,
                        loaded->format_id.to_string()
                    );
                    ctx.world().emit_event<ResourceLoadedComponent>( { loaded->handle, loaded->format_id }, id );
                }
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

void Scene::ensure_root_node_exists_()
{
    if (root_node)
        return;

    root_node            = node_pool.acquire();
    root_node->scene     = this;
    root_node->node_pool = &node_pool;
    root_node->internal_id =
        ecs_world.entity( "RootNode" ).with<NodeRefComponent>( { root_node } ).with<RootNodeTag>().build();

    root_node->child_query = ecs_world.query( "RootNode_ChildQuery" )
                                 .with<NodeRefComponent>()
                                 .with_pair( static_cast<EcsEntity>( EcsChildOf ), root_node->internal_id )
                                 .build();

    NC_LOG_TRACE( "Root node was missing but is now created with entity ID {}", root_node->internal_id );
}

} // namespace nc
