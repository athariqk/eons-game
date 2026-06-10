#include "OrganismAISystem.h"

#include <Logger.h>
#include <Random.h>
#include <RigidBodyComponent.h>
#include <TransformComponent.h>
#include <World.h>

#include "Genes.h"
#include "MicrocosmWorld.h"
#include "components/FoodComponent.h"
#include "components/OrganismAIComponent.h"
#include "components/OrganismComponent.h"
#include "components/SpeciesComponent.h"

bool OrganismAISystem::on_init(ncore::World &world) {
    for (const auto &entity_ptr: world.get_entities()) {
        if (!entity_ptr->is_enabled() || !entity_ptr->has_component<OrganismAIComponent>())
            continue;

        auto &ai = entity_ptr->get_component<OrganismAIComponent>();

        if (entity_ptr->has_component<ncore::TransformComponent>()) {
            change_state(ai, BehaviourState::IDLING);
        } else {
            LOG_ERROR(ncore::log::GAME, "Organism AI entity missing transform component!");
        }
    }
    return true;
}

void OrganismAISystem::on_fixed_update(ncore::World &world, double fixedDelta) {
    float delta = static_cast<float>(fixedDelta);

    for (const auto &entity_ptr: world.get_entities()) {
        if (!entity_ptr->is_enabled() || !entity_ptr->has_component<OrganismAIComponent>() ||
            !entity_ptr->has_component<OrganismComponent>())
            continue;

        auto &ai = entity_ptr->get_component<OrganismAIComponent>();
        auto &organism = entity_ptr->get_component<OrganismComponent>();

        // Organic metabolism: Constant energy burn to prevent perpetual motion
        organism.cur_energy -= 1.5f * delta;
        if (organism.cur_energy < 0.0f)
            organism.cur_energy = 0.0f;

        if (organism.cur_energy > organism.genome.energy_capacity / 2.0f) {
            ai.reproduce_interval += 0.1f * delta;
        }

        switch (ai.state) {
            case BehaviourState::IDLING:
                update_idling(ai, delta);
                break;
            case BehaviourState::RUN_TUMBLE:
                update_run_and_tumble(*entity_ptr, ai, organism, world, delta);
                break;
            case BehaviourState::ABSORBING:
                update_absorbing(*entity_ptr, ai, organism, delta);
                break;
            case BehaviourState::EVALUATE:
                update_eval(*entity_ptr, ai, organism, world, delta);
                break;
            default:
                break;
        }
    }
}

void OrganismAISystem::change_state(OrganismAIComponent &ai, BehaviourState newState) {
    if (ai.state == BehaviourState::ABSORBING) {
        ai.is_absorbing = false;
        if (ai.captured_food) {
            ai.captured_food->caught = false;
            ai.captured_food = nullptr;
        }
    }

    ai.state = newState;
    ai.act_timer = 0.0f;

    if (newState == BehaviourState::IDLING) {
        ai.has_moved = false;
    } else if (newState == BehaviourState::ABSORBING) {
        ai.is_absorbing = true;
    }
}

void OrganismAISystem::update_idling(OrganismAIComponent &ai, float delta) {
    ai.act_timer += delta;

    // Desynchronized idling so swarms look organic
    if (ai.act_timer > ncore::Random::rand_float(0.5f, 1.5f)) {
        change_state(ai, BehaviourState::RUN_TUMBLE);
    }
}

void OrganismAISystem::update_run_and_tumble(ncore::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
                                          ncore::World &world, float delta) {
    ai.act_timer += delta;

    auto &transform = entity.get_component<ncore::TransformComponent>();
    MicrocosmWorld *micro_world = static_cast<MicrocosmWorld *>(&world);
    auto &foods = micro_world->get_group(MicrocosmWorld::GroupLabels::NUTRIENTS_GROUP);

    float sight_dist_sqr = 0.5f * 0.5f;
    float absorb_dist_sqr = 0.5f * 0.5f;
    float closest_dist_sqr = sight_dist_sqr;

    FoodComponent *closest_food = nullptr;
    ncore::Vec2D target_pos;
    bool smells_food = false;

    // Scan environment
    for (auto *food_entity: foods) {
        if (!food_entity->is_enabled() || !food_entity->has_component<FoodComponent>())
            continue;

        auto &nutrient = food_entity->get_component<FoodComponent>();
        if (nutrient.caught || !food_entity->has_component<ncore::TransformComponent>())
            continue;

        auto &food_tsfm = food_entity->get_component<ncore::TransformComponent>();
        float dist_sqr = (transform.position - food_tsfm.position).length_sqr();

        if (dist_sqr < closest_dist_sqr) {
            closest_dist_sqr = dist_sqr;
            closest_food = &nutrient;
            target_pos = food_tsfm.position;
            smells_food = true;
        }
    }

    // Apply Biased Movement (Chemotaxis)
    if (!ai.has_moved && entity.has_component<ncore::RigidBodyComponent>()) {
        ai.has_moved = true;
        auto &body = entity.get_component<ncore::RigidBodyComponent>();

        ncore::Vec2D impulse;
        bool is_hungry = organism.cur_energy < organism.genome.energy_capacity * 0.8f;

        if (smells_food && is_hungry) {
            ncore::Vec2D dir_to_food = target_pos - transform.position;
            float length = dir_to_food.length();
            if (length > 0.001f) {
                dir_to_food = dir_to_food / length;
            }

            // Blend perfect trajectory with a 40% organic wobble
            ncore::Vec2D wobble = get_rand_dir() * 0.4f;
            ncore::Vec2D final_dir = dir_to_food + wobble;

            float final_len = final_dir.length();
            if (final_len > 0.001f)
                final_dir = final_dir / final_len;

            impulse = final_dir * ai.move_speed;
        } else {
            impulse = get_rand_dir() * ai.move_speed;
        }

        body.pending_impulse = impulse;
    }

    // State Transitions
    if (smells_food && closest_dist_sqr < absorb_dist_sqr && organism.cur_energy < organism.genome.energy_capacity * 0.8f) {
        ai.is_food_found = true;
        ai.captured_food = closest_food;
        change_state(ai, BehaviourState::ABSORBING);
    } else if (ai.act_timer > ai.act_interval) {
        change_state(ai, BehaviourState::IDLING);
    }
}

void OrganismAISystem::update_absorbing(ncore::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
                                       float delta) {
    absorb_food(entity, ai, organism);
    ai.act_timer += delta;

    if (!ai.is_food_found) {
        change_state(ai, BehaviourState::RUN_TUMBLE);
    } else if (ai.act_timer > ai.act_interval * 5.0f) {
        ai.is_food_found = false;
        change_state(ai, BehaviourState::EVALUATE);
    }
}

void OrganismAISystem::update_eval(ncore::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
                                      ncore::World &world, float delta) {
    ai.act_timer += delta;

    if (organism.cur_energy > organism.genome.energy_capacity * 0.7f && ai.reproduce_interval > 100.0f) {
        ai.reproduced = false;
        LOG_INFO(ncore::log::GAME, "Organism {} reproduced", organism.entity->get_id());
        reproduce(world, entity, ai, organism);
        change_state(ai, BehaviourState::RUN_TUMBLE);
    } else if (ai.act_timer > ai.act_interval * 2.0f) {
        change_state(ai, BehaviourState::RUN_TUMBLE);
    }
}

ncore::Vec2D OrganismAISystem::get_rand_dir() {
    float angle = ncore::Random::rand_float(0.0f, 2.0f * M_PI);
    return ncore::Vec2D(std::cos(angle), std::sin(angle));
}

void OrganismAISystem::absorb_food(ncore::Entity &entity, OrganismAIComponent &ai, OrganismComponent &organism) {
    if (ai.captured_food == nullptr) {
        LOG_ERROR(ncore::log::GAME, "Nutrient is not found while trying to absorb it!");
        ai.is_food_found = false;
        return;
    }

    auto &transform = entity.get_component<ncore::TransformComponent>();

    ai.captured_food->caught = true;
    ai.captured_food->eater_pos = transform.position;

    if (organism.cur_energy < organism.genome.energy_capacity) {
        if (ai.captured_food->cur_energy > 0.0f) {
            organism.cur_energy += ai.absorb_speed;
        }

        ai.captured_food->cur_energy -= ai.absorb_speed;
        organism.fitness += 0.05f;
    }
}

void OrganismAISystem::reproduce(ncore::World &world, ncore::Entity &entity, OrganismAIComponent &ai,
                                 OrganismComponent &organism) {
    if (ai.reproduced)
        return;

    ai.reproduced = true;
    ai.reproduce_interval = 0.0f;

    MicrocosmWorld &micro_world = static_cast<MicrocosmWorld &>(world);
    auto &offspring = micro_world.add_organism(micro_world.get_species_by_id(organism.species_id));
    LOG_INFO(ncore::log::GAME, "Organism {} has been birthed into the world", offspring.entity->get_id());

    if (offspring.entity && offspring.entity->has_component<ncore::TransformComponent>()) {
        auto &transform = entity.get_component<ncore::TransformComponent>();
        auto &offspring_tsfm = offspring.entity->get_component<ncore::TransformComponent>();
        offspring_tsfm.position = transform.position;
    }

    // Do random mutations
    if (offspring.entity && offspring.genome.mutate(5, 1)) {
        LOG_INFO(ncore::log::GAME, "Mutation has occurred on organism {}", offspring.entity->get_id());
    }

    organism.cur_energy -= 15.0f; // childbirth tax
}

