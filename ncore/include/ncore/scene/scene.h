#pragma once

#include <ncore/core/collection.h>
#include <ncore/game_world.h>
#include <ncore/runtime/ecs_world.h>
#include <ncore/scene/node.h>

namespace nc {

struct AppDesc;
class ModuleRegistry;
class Node;

/**
 * @brief Scene defines the default IGameWorld implementation where game objects are
 * represented as a hierarchical node structure. A scene contains a root Node which
 * can have many child Nodes, and each Node can have multiple components attached to it.
 * You define game logic by... i don't know, lets figure it out :P
 *
 * This was very inspired by Godot's Node system.
 *
 * In practice, this is just a thin layer of abstraction on top of EcsWorld which is
 * a pure ECS runtime and may not be easily approachable to most game developers.
 * Therefore, with this we can at least make game authoring a little bit easier.
 */
class NCAPI Scene : public IGameWorld {
    NCLASS( Scene, IGameWorld )

    using NodePool                              = PagedPool<Node>;
    static constexpr EcsEntityId ROOT_PARENT_ID = 0xABCDEF123FFFFF;

public:
    Scene( AppDesc& p_app_desc, ModuleRegistry& p_modules );

    Scene( const Scene& )            = delete;
    Scene& operator=( const Scene& ) = delete;

    void on_init() override;
    bool on_fixed_update( double p_delta ) override;
    bool on_variable_update( double p_delta ) override;
    void on_finish() override;

    /**
     * @brief This may be used for low-level accesss to the ECS runtime.
     */
    EcsWorld& get_ecs()
    {
        return ecs_world;
    }

    /**
     * @brief This creates a new root if current one doesn't exist.
     */
    Node* get_root_node();

private:
    void ensure_root_node_exists_();

private:
    friend class Node;

    EcsWorld ecs_world;
    NodePool node_pool;
    Node* root_node = nullptr;
};

} // namespace nc
