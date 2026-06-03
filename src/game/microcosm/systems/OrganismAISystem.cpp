#include "OrganismAISystem.h"

#include <Logger.h>
#include <Random.h>
#include <RigidBodyComponent.h>
#include <TransformComponent.h>
#include <World.h>

#include "Genes.h"
#include "MicrocosmWorld.h"
#include "components/NutrientComponent.h"
#include "components/OrganismAIComponent.h"
#include "components/OrganismComponent.h"
#include "components/SpeciesComponent.h"

bool OrganismAISystem::OnInit(Aeon::World &world) {
    for (const auto &entityPtr: world.GetEntities()) {
        if (!entityPtr->IsEnabled() || !entityPtr->HasComponent<OrganismAIComponent>())
            continue;

        auto &ai = entityPtr->GetComponent<OrganismAIComponent>();

        if (entityPtr->HasComponent<Aeon::TransformComponent>()) {
            ChangeState(ai, BehaviourState::Idling);
        } else {
            LOG_ERROR("Organism AI entity missing transform component!");
        }
    }
    return true;
}

void OrganismAISystem::OnFixedUpdate(Aeon::World &world, double fixedDelta) {
    float delta = static_cast<float>(fixedDelta);

    for (const auto &entityPtr: world.GetEntities()) {
        if (!entityPtr->IsEnabled() || !entityPtr->HasComponent<OrganismAIComponent>() ||
            !entityPtr->HasComponent<OrganismComponent>())
            continue;

        auto &ai = entityPtr->GetComponent<OrganismAIComponent>();
        auto &organism = entityPtr->GetComponent<OrganismComponent>();

        // Organic metabolism: Constant energy burn to prevent perpetual motion
        organism.curEnergy -= 1.5f * delta;
        if (organism.curEnergy < 0.0f)
            organism.curEnergy = 0.0f;

        if (organism.curEnergy > organism.genome.energyCapacity / 2.0f) {
            ai.reproduceInterval += 0.1f * delta;
        }

        switch (ai.behaviourState) {
            case BehaviourState::Idling:
                UpdateIdling(ai, delta);
                break;
            case BehaviourState::RunAndTumble:
                UpdateRunAndTumble(*entityPtr, ai, organism, world, delta);
                break;
            case BehaviourState::Absorbing:
                UpdateAbsorbing(*entityPtr, ai, organism, delta);
                break;
            case BehaviourState::Evaluate:
                UpdateEvaluate(*entityPtr, ai, organism, world, delta);
                break;
            default:
                break;
        }
    }
}

void OrganismAISystem::ChangeState(OrganismAIComponent &ai, BehaviourState newState) {
    if (ai.behaviourState == BehaviourState::Absorbing) {
        ai.isAbsorbing = false;
        if (ai.caughtNutrient) {
            ai.caughtNutrient->caught = false;
            ai.caughtNutrient = nullptr;
        }
    }

    ai.behaviourState = newState;
    ai.actTimer = 0.0f;

    if (newState == BehaviourState::Idling) {
        ai.hasMoved = false;
    } else if (newState == BehaviourState::Absorbing) {
        ai.isAbsorbing = true;
    }
}

void OrganismAISystem::UpdateIdling(OrganismAIComponent &ai, float delta) {
    ai.actTimer += delta;

    // Desynchronized idling so swarms look organic
    if (ai.actTimer > Aeon::Random::RandomFloat(0.5f, 1.5f)) {
        ChangeState(ai, BehaviourState::RunAndTumble);
    }
}

void OrganismAISystem::UpdateRunAndTumble(Aeon::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
                                          Aeon::World &world, float delta) {
    ai.actTimer += delta;

    auto &transform = entity.GetComponent<Aeon::TransformComponent>();
    MicrocosmWorld *microWorld = static_cast<MicrocosmWorld *>(&world);
    auto &nutrients = microWorld->GetGroup(MicrocosmWorld::GroupLabels::NutrientsGroup);

    float sightDistSqr = 0.5f * 0.5f;
    float absorbDistSqr = 0.5f * 0.5f;
    float closestDistSqr = sightDistSqr;

    NutrientComponent *closestNutrient = nullptr;
    Aeon::Vector2D targetPos;
    bool smellsFood = false;

    // Scan environment
    for (auto *nutrientEntity: nutrients) {
        if (!nutrientEntity->IsEnabled() || !nutrientEntity->HasComponent<NutrientComponent>())
            continue;

        auto &nutrient = nutrientEntity->GetComponent<NutrientComponent>();
        if (nutrient.caught || !nutrientEntity->HasComponent<Aeon::TransformComponent>())
            continue;

        auto &nutTrans = nutrientEntity->GetComponent<Aeon::TransformComponent>();
        float distSqr = (transform.position - nutTrans.position).LengthSqr();

        if (distSqr < closestDistSqr) {
            closestDistSqr = distSqr;
            closestNutrient = &nutrient;
            targetPos = nutTrans.position;
            smellsFood = true;
        }
    }

    // Apply Biased Movement (Chemotaxis)
    if (!ai.hasMoved && entity.HasComponent<Aeon::RigidBodyComponent>()) {
        ai.hasMoved = true;
        auto &body = entity.GetComponent<Aeon::RigidBodyComponent>();

        Aeon::Vector2D impulse;
        bool isHungry = organism.curEnergy < organism.genome.energyCapacity * 0.8f;

        if (smellsFood && isHungry) {
            Aeon::Vector2D dirToFood = targetPos - transform.position;
            float length = dirToFood.Length();
            if (length > 0.001f) {
                dirToFood = dirToFood / length;
            }

            // Blend perfect trajectory with a 40% organic wobble
            Aeon::Vector2D wobble = GetRandomDirection() * 0.4f;
            Aeon::Vector2D finalDir = dirToFood + wobble;

            float finalLength = finalDir.Length();
            if (finalLength > 0.001f)
                finalDir = finalDir / finalLength;

            impulse = finalDir * ai.moveSpeed;
        } else {
            impulse = GetRandomDirection() * ai.moveSpeed;
        }

        body.pendingImpulse = impulse;
    }

    // State Transitions
    if (smellsFood && closestDistSqr < absorbDistSqr && organism.curEnergy < organism.genome.energyCapacity * 0.8f) {
        ai.isNutrientFound = true;
        ai.caughtNutrient = closestNutrient;
        ChangeState(ai, BehaviourState::Absorbing);
    } else if (ai.actTimer > ai.actInterval) {
        ChangeState(ai, BehaviourState::Idling);
    }
}

void OrganismAISystem::UpdateAbsorbing(Aeon::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
                                       float delta) {
    AbsorbNutrient(entity, ai, organism);
    ai.actTimer += delta;

    if (!ai.isNutrientFound) {
        ChangeState(ai, BehaviourState::RunAndTumble);
    } else if (ai.actTimer > ai.actInterval * 5.0f) {
        ai.isNutrientFound = false;
        ChangeState(ai, BehaviourState::Evaluate);
    }
}

void OrganismAISystem::UpdateEvaluate(Aeon::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
                                      Aeon::World &world, float delta) {
    ai.actTimer += delta;

    if (organism.curEnergy > organism.genome.energyCapacity * 0.7f && ai.reproduceInterval > 100.0f) {
        ai.reproduced = false;
        Reproduce(world, entity, ai, organism);
        ChangeState(ai, BehaviourState::RunAndTumble);
    } else if (ai.actTimer > ai.actInterval * 2.0f) {
        ChangeState(ai, BehaviourState::RunAndTumble);
    }
}

Aeon::Vector2D OrganismAISystem::GetRandomDirection() {
    float angle = Aeon::Random::RandomFloat(0.0f, 2.0f * 3.14159265f);
    return Aeon::Vector2D(std::cos(angle), std::sin(angle));
}

void OrganismAISystem::AbsorbNutrient(Aeon::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism) {
    if (ai.caughtNutrient == nullptr) {
        LOG_ERROR("Nutrient is not found while trying to absorb it!");
        ai.isNutrientFound = false;
        return;
    }

    auto &transform = entity.GetComponent<Aeon::TransformComponent>();

    ai.caughtNutrient->caught = true;
    ai.caughtNutrient->organismPos = transform.position;

    if (organism.curEnergy < organism.genome.energyCapacity) {
        if (ai.caughtNutrient->curEnergy > 0.0f) {
            organism.curEnergy += ai.absorbSpeed;
        }

        ai.caughtNutrient->curEnergy -= ai.absorbSpeed;
        organism.fitness += 0.05f;
    }
}

void OrganismAISystem::Reproduce(Aeon::World &world, Aeon::Entity &entity, OrganismAIComponent &ai,
                                 OrganismComponent &organism) {
    if (ai.reproduced)
        return;

    ai.reproduced = true;
    ai.reproduceInterval = 0.0f;

    MicrocosmWorld &microWorld = static_cast<MicrocosmWorld &>(world);
    auto &offspring = microWorld.AddOrganism(microWorld.GetSpeciesById(organism.speciesId));

    if (offspring.entity && offspring.entity->HasComponent<Aeon::TransformComponent>()) {
        auto &transform = entity.GetComponent<Aeon::TransformComponent>();
        auto &offspringTransform = offspring.entity->GetComponent<Aeon::TransformComponent>();
        offspringTransform.position = transform.position;
    }

    organism.curEnergy -= 15.0f; // Childbirth tax
    LOG_INFO("Organism {} reproduced", organism.entity->GetID());
}
