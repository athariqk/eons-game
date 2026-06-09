#pragma once

#include <System.h>
#include <Vector2D.h>

#include "components/OrganismAIComponent.h"

namespace Aeon {
class World;
class Entity;
} // namespace Aeon

struct OrganismComponent;

class OrganismAISystem : public Aeon::System {
public:
    OrganismAISystem() { SetPriority(55); }

    bool OnInit(Aeon::World &world) override;
    void OnFixedUpdate(Aeon::World &world, double fixedDelta) override;

private:
    Aeon::Vector2D GetRandomDirection();

    // FSM Core
    void ChangeState(OrganismAIComponent &ai, BehaviourState newState);

    // State Handlers
    void UpdateIdling(OrganismAIComponent &ai, float delta);
    void UpdateRunAndTumble(Aeon::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
                            Aeon::World &world, float delta);
    void UpdateAbsorbing(Aeon::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism, float delta);
    void UpdateEvaluate(Aeon::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism, Aeon::World &world,
                        float delta);

    // Action Handlers
    void AbsorbNutrient(Aeon::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism);
    void Reproduce(Aeon::World &world, Aeon::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism);
};
