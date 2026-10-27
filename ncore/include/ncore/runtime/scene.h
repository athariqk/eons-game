#pragma once

#include <functional>

#include <ncore/core/collection.h>
#include <ncore/game_world.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/resources/resource_loader.h>

#include "node.h"

namespace nc {

struct AppDesc;
class ServiceRegistry;

/**
 * @brief ECS tag to mark the Scene's root node.
 */
NC_COMPONENT_API( RootNodeTag ){};

NC_COMPONENT_API( NodeRefComponent )
{
    REFLECT Node* node = nullptr;
};

/**
 * @brief Scene defines the default IGameWorld implementation where game objects are
 * represented as a hierarchical node structure. A scene contains a root Node which
 * can have many child Nodes, and each Node can have multiple components attached to it,
 * which defines its "type" and thus archetype. You compose game logic by defining systems
 * over these archetypes via the Scene API.
 *
 * In practice, this is just a thin layer of abstraction on top of EcsWorld which is
 * a pure ECS runtime and may not be easily approachable to most game developers.
 * Therefore, with this we can at least make game authoring a little bit easier.
 *
 * Scene by default loads a feature bundle which provides
 * NCORE Engine functionalities for the ECS runtime.
 * This includes but not limited to: rendering, physics, and
 * others. You may bypass this altogether and just load whichever
 * features you need by overriding Scene.on_enter().
 *
 * This was very inspired by Godot's Node system.
 */
class NCAPI Scene : public IGameWorld {
    NCLASS( Scene, IGameWorld )

public:
    Scene() = default;

    Scene( const Scene& )            = delete;
    Scene& operator=( const Scene& ) = delete;

    void on_enter() override;
    void on_ready() override;
    bool on_fixed_update( double p_delta ) override;
    bool on_variable_update( double p_delta ) override;
    void on_exit() override;

    template<class... Comps, typename Fn>
    void add_system( Fn&& p_callback, EcsSystemPhase p_phase, int p_order = 0 )
    {
        auto name = std::format( "Scene_Sys_{}", ++system_counter );
        ecs_world.system( name ).with<NodeRefComponent, Comps...>().in( p_phase ).order( p_order ).each(
            [fn = std::forward<Fn>( p_callback )]( EcsIterState& ctx ) {
                auto ref = ctx.get_component<NodeRefComponent>();
                NC_ASSERT_MSG( ref && ref->node, "Entity missing NodeRefComponent" );
                fn( *ref->node, ( *ctx.get_component<Comps>() )..., ctx.delta_time() );
            }
        );
    }

    template<typename Fn>
    void add_startup( Fn&& p_callback, int p_order = 0 )
    {
        auto name = std::format( "Scene_Startup_{}", ++system_counter );
        ecs_world.system( name )
            .in( EcsSystemPhase::INIT )
            .order( p_order )
            .run( [fn = std::forward<Fn>( p_callback )]( EcsIterState& ctx ) { fn(); } );
    }

    template<typename T, typename Fn>
    void add_observer( Fn&& p_callback, EcsComponent p_event )
    {
        ecs_world.observer( std::format( "Scene_Obs_{}", ++system_counter ) )
            .with<NodeRefComponent>()
            .on<T>( p_event )
            .each( [fn = std::forward<Fn>( p_callback )]( EcsIterState& ctx ) {
                auto ref = ctx.get_component<NodeRefComponent>();
                fn( *ref->node, *ctx.get_component<T>() );
            } );
    }

    /**
     * @brief This may be used for low-level access to the ECS runtime.
     */
    EcsWorld& get_ecs() [[clang::lifetimebound]]
    {
        return ecs_world;
    }

    /**
     * @brief Access the per-scene resource loader.
     */
    ResourceLoader& get_resource_loader() [[clang::lifetimebound]]
    {
        return resource_loader;
    }

    /**
     * @brief Get the root node of this scene.
	 *
	 * This creates a new root node if current one doesn't exist.
     */
    Node* root();

    /**
     * @brief Checks whether a node is not being destroyed.
     */
    bool is_node_valid( Node* p_node );

    /**
	 * @brief Get the Node associated with an entity ID, or nullptr if the entity is invalid or has no Node.
     */
    Node* node_from_entity( EcsEntity p_entity );

    /**
     * @brief Safely defer the deletion of a Node at the next frame.
     */
    void queue_destroy_node( Node* p_node );

    /**
     * @brief Queue work that must run outside the ECS progress (structural changes).
     *
     * Systems execute with the world read-only, so spawning/destroying entities
     * from a system callback only yields deferred (temporary) ids. Editor and
     * gameplay code that reacts to input queues the work here instead; it runs
     * at the start of the next variable update, before cs_world.progress().
     */
    void queue_task( std::function<void()> p_task );

    /** @brief
	* Generate a unique display name for an unnamed child node.
	*/
    String generate_node_name();

private:
    void ensure_root_node_exists_();
    void process_pending_tasks_();
    void process_pending_node_deletions_();

    EcsWorld ecs_world;
    ResourceLoader resource_loader;
    Node::NodePool node_pool;
    Node* root_node            = nullptr;
    size_t system_counter      = 0;
    uint64_t node_name_counter = 0;
    DynamicArray<std::function<void()>> pending_tasks;
    HashSet<Node*> pending_node_deletions; // TODO: any way to do this efficiently would be nice
};

} // namespace nc
