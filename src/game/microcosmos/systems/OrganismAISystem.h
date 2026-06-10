#pragma once

#include <System.h>
#include <Vec2D.h>

#include "components/OrganismAIComponent.h"

namespace ncore {
class World;
class Entity;
} // namespace ncore

struct OrganismComponent;

class OrganismAISystem : public ncore::System {
public:
    OrganismAISystem() { set_priority(55); }

    bool on_init(ncore::World &world) override;
    void on_fixed_update(ncore::World &world, double fixedDelta) override;

private:
    ncore::Vec2D get_rand_dir();

    // FSM Core
    void change_state(OrganismAIComponent &ai, BehaviourState newState);

    // State Handlers
    void update_idling(OrganismAIComponent &ai, float delta);
    void update_run_and_tumble(ncore::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
                            ncore::World &world, float delta);
    void update_absorbing(ncore::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism, float delta);
    void update_eval(ncore::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism, ncore::World &world,
                        float delta);

    // Action Handlers
    void absorb_food(ncore::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism);
    void reproduce(ncore::World &world, ncore::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism);
};
