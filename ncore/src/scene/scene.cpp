#include <ncore/modules/module_registry.h>
#include <ncore/runtime/components/ecs_time.h>
#include <ncore/runtime/ecs_base_features.h>
#include <ncore/scene/node.h>
#include <ncore/scene/scene.h>

namespace nc {

Scene::Scene( AppDesc& p_app_desc, ModuleRegistry& p_modules ) : IGameWorld( p_app_desc, p_modules ), ecs_world( *this )
{}

void Scene::on_init()
{
    ecs_world.emplace_singleton<EcsTime>();
    ecs_world.load_feature<EcsBaseFeatures>( app_desc );
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
    NC_LOG_TRACE_C( log::ECS, "Scene teardown" );
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
    NC_LOG_TRACE( "Root node was missing but is now created with entity ID {}", root_node->internal_id );
}

} // namespace nc
