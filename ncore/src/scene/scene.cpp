#include <ncore/modules/events/event_bus.h>
#include <ncore/modules/service_locator.h>
#include <ncore/scene/node.h>
#include <ncore/scene/scene.h>

namespace ncore {

Scene::Scene( ServiceLocator& services ) : IGameWorld( services ), ecs_world() {}

void Scene::on_init()
{
    auto event_bus = get_services().resolve<EventBus>();

    event_bus->subscribe<WindowCloseEvent>( [this]( WindowCloseEvent& ) {
        wants_to_quit = true;
        NC_LOG_TRACE( "window close event received, requesting exit..." );
    } );

    event_bus->subscribe<WindowResizeEvent>( [this]( WindowResizeEvent& e ) {
        NC_LOG_TRACE( "window resolution changed: {}x{}", e.width, e.height );
    } );

    ensure_root_node_exists_();
}

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

void Scene::on_finish()
{
    NC_LOG_TRACE_C( log::ECS, "world finished" );
}

Node* Scene::get_root_node()
{
    ensure_root_node_exists_();
    return root_node;
}

void Scene::ensure_root_node_exists_()
{
    if (root_node)
        return;
    root_node = node_pool.acquire();
    root_node->set_scene( this );
    root_node->internal_id = ecs_world.create_entity( "RootNode" ).build();
    NC_LOG_TRACE( "root node was missing but is now created with entity ID {}", root_node->internal_id );
}

} // namespace ncore
