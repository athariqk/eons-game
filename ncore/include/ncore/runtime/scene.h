#pragma once

#include <ncore/core/collection.h>
#include <ncore/game_world.h>
#include <ncore/runtime/ecs/ecs_world.h>

#include "node.h"

namespace nc {

struct AppDesc;
class ServiceRegistry;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"
// ECS tag for the root node.
struct NCAPI RootNodeTag {
    NSTRUCT( RootNodeTag )
};
#pragma clang diagnostic pop

struct NCAPI NodeRefComponent {
    Node* node = nullptr;
    NSTRUCT( NodeRefComponent, NC_F( NodeRefComponent, node ) )
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
    Scene( AppDesc& p_app_desc, ServiceRegistry& p_services );

    Scene( const Scene& )            = delete;
    Scene& operator=( const Scene& ) = delete;

    void on_enter() override;
    void on_ready() override;
    bool on_fixed_update( double p_delta ) override;
    bool on_variable_update( double p_delta ) override;
    void on_exit() override;

    template<class... Comps, typename Fn>
    void add_system( Fn&& callback, EcsSystemPhase phase, int order = 0 )
    {
        auto name = std::format( "Scene_Sys_{}", ++system_counter );
        ecs_world.system( name ).with<NodeRefComponent, Comps...>().in( phase ).order( order ).each(
            [fn = std::forward<Fn>( callback )]( QueryContext& ctx, EcsEntity ) {
                auto ref = ctx.get_component<NodeRefComponent>();
                NC_ASSERT( ref && ref->node, "Entity missing NodeRefComponent" );
                fn( *ref->node, ( *ctx.get_component<Comps>() )..., ctx.delta_time() );
            }
        );
    }

    template<typename Fn>
    void add_startup( Fn&& callback, int order = 0 )
    {
        auto name = std::format( "Scene_Startup_{}", ++system_counter );
        ecs_world.system( name )
            .in( EcsSystemPhase::INIT )
            .order( order )
            .each( [fn = std::forward<Fn>( callback )]( QueryContext& ctx, EcsEntity ) { fn(); } );
    }

    template<typename T, typename Fn>
    void add_observer( Fn&& callback, EcsComponent event )
    {
        ecs_world.observer( std::format( "Scene_Obs_{}", ++system_counter ) )
            .with<NodeRefComponent>()
            .on<T>( event )
            .each( [fn = std::forward<Fn>( callback )]( QueryContext& ctx, EcsEntity ) {
                auto ref = ctx.get_component<NodeRefComponent>();
                fn( *ref->node, *ctx.get_component<T>() );
            } );
    }

    /**
     * @brief This may be used for low-level access to the ECS runtime.
     */
    EcsWorld& get_ecs()
    {
        return ecs_world;
    }

    /**
     * @brief This creates a new root node if current one doesn't exist.
     */
    Node* root();

    bool is_node_valid( Node* node );

    /**
     * @brief Safely defer the deletion of a Node at the next frame.
     */
    void queue_destroy_node( Node* node );

private:
    void ensure_root_node_exists_();
    void process_pending_node_deletions_();

    EcsWorld ecs_world;
    Node::NodePool node_pool;
    Node* root_node       = nullptr;
    size_t system_counter = 0;
    DynamicArray<Node*> pending_node_deletions; // TODO: any way to do this without DynamicArray would be nice
};

} // namespace nc
