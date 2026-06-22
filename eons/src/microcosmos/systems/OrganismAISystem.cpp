#include "OrganismAISystem.h"

#include <numbers>

#include <ncore/kernel/random.h>
#include <ncore/modules/ecs/ecs_world.h>
#include <ncore/utils/log.h>

#include <microcosmos/GameGroups.h>
#include <microcosmos/Genes.h>
#include <microcosmos/OrganismFactory.h>
#include <microcosmos/SpeciesRegistry.h>
#include <microcosmos/components/FoodComponent.h>
#include <microcosmos/components/OrganismAIComponent.h>
#include <microcosmos/components/OrganismComponent.h>
#include <microcosmos/components/SpeciesComponent.h>

// void OrganismAISystem::on_init(ncore::EcsWorld &world) {
//  for (const auto &entity: world.get_entities()) {
//      if (!entity.is_enabled || !world.has<OrganismAIComponent>(entity))
//          continue;

//    auto &ai = world.get<OrganismAIComponent>(entity);

//    if (world.has<ncore::EcsTransform>(entity))
//        change_state(ai, BehaviourState::IDLING);
//    else
//        NC_LOG_ERROR("Organism AI entity missing transform component!");
//}
//}

// void OrganismAISystem::on_process(ncore::EcsWorld &world, double delta_time) {
//     float delta = static_cast<float>(delta_time);

// for (const auto &entity: world.get_entities()) {
//     if (!entity.is_enabled || !world.has<OrganismAIComponent>(entity) || !world.has<OrganismComponent>(entity))
//         continue;

//    auto &ai = world.get<OrganismAIComponent>(entity);
//    auto &organism = world.get<OrganismComponent>(entity);

//    organism.cur_energy -= 1.5f * delta;
//    if (organism.cur_energy < 0.0f)
//        organism.cur_energy = 0.0f;

//    if (organism.cur_energy > organism.genome.energy_capacity / 2.0f)
//        ai.reproduce_interval += 0.1f * delta;

//    switch (ai.state) {
//        case BehaviourState::IDLING:
//            update_idling(ai, delta);
//            break;
//        case BehaviourState::RUN_TUMBLE:
//            update_run_and_tumble(const_cast<ncore::EcsEntity &>(entity), ai, organism, world, delta);
//            break;
//        case BehaviourState::ABSORBING:
//            update_absorbing(world, const_cast<ncore::EcsEntity &>(entity), ai, organism, delta);
//            break;
//        case BehaviourState::EVALUATE:
//            update_eval(const_cast<ncore::EcsEntity &>(entity), ai, organism, world, delta);
//            break;
//        default:
//            break;
//    }
//}
//}

// void OrganismAISystem::change_state(OrganismAIComponent &ai, BehaviourState newState) {
//  if (ai.state == BehaviourState::ABSORBING) {
//      ai.is_absorbing = false;
//      if (ai.captured_food) {
//          ai.captured_food->caught = false;
//          ai.captured_food = nullptr;
//      }
//  }

// ai.state = newState;
// ai.act_timer = 0.0f;

// if (newState == BehaviourState::IDLING)
//     ai.has_moved = false;
// else if (newState == BehaviourState::ABSORBING)
//     ai.is_absorbing = true;
//}

// void OrganismAISystem::update_idling(OrganismAIComponent &ai, double delta) {
//     ai.act_timer += delta;
//
//     if (ai.act_timer > ncore::Random::rand_float(0.5f, 1.5f))
//         change_state(ai, BehaviourState::RUN_TUMBLE);
// }
//
// void OrganismAISystem::update_run_and_tumble(ncore::EcsEntity &entity, OrganismAIComponent &ai,
//                                              OrganismComponent &organism, ncore::EcsWorld &world, double delta) {
//  ai.act_timer += delta;

// auto &transform = world.get<ncore::TransformComponent>(entity);
// auto &foods = world.get_group(GameGroups::NUTRIENTS_GROUP);

// float sight_dist_sqr = 0.5f * 0.5f;
// float absorb_dist_sqr = 0.5f * 0.5f;
// float closest_dist_sqr = sight_dist_sqr;

// FoodComponent *closest_food = nullptr;
// ncore::Vec2 target_pos;
// bool smells_food = false;

// for (auto *food_entity: foods) {
//     if (!food_entity->is_enabled || !world.has<FoodComponent>(*food_entity))
//         continue;

//    auto &nutrient = world.get<FoodComponent>(*food_entity);
//    if (nutrient.caught || !world.has<ncore::TransformComponent>(*food_entity))
//        continue;

//    auto &food_tsfm = world.get<ncore::TransformComponent>(*food_entity);
//    float dist_sqr = (transform.position - food_tsfm.position).length_sqr();

//    if (dist_sqr < closest_dist_sqr) {
//        closest_dist_sqr = dist_sqr;
//        closest_food = &nutrient;
//        target_pos = food_tsfm.position;
//        smells_food = true;
//    }
//}

// if (!ai.has_moved && world.has<ncore::RigidbodyComponent>(entity)) {
//     ai.has_moved = true;
//     auto &body = world.get<ncore::RigidbodyComponent>(entity);

//    ncore::Vec2 impulse;
//    bool is_hungry = organism.cur_energy < organism.genome.energy_capacity * 0.8f;

//    if (smells_food && is_hungry) {
//        ncore::Vec2 dir_to_food = target_pos - transform.position;
//        float length = dir_to_food.length();
//        if (length > 0.001f)
//            dir_to_food = dir_to_food / length;

//        ncore::Vec2 wobble = get_rand_dir() * 0.4f;
//        ncore::Vec2 final_dir = dir_to_food + wobble;

//        float final_len = final_dir.length();
//        if (final_len > 0.001f)
//            final_dir = final_dir / final_len;

//        impulse = final_dir * ai.move_speed;
//    } else {
//        impulse = get_rand_dir() * ai.move_speed;
//    }

//    body.pending_impulse = impulse;
//}

// if (smells_food && closest_dist_sqr < absorb_dist_sqr &&
//     organism.cur_energy < organism.genome.energy_capacity * 0.8f) {
//     ai.is_food_found = true;
//     ai.captured_food = closest_food;
//     change_state(ai, BehaviourState::ABSORBING);
// } else if (ai.act_timer > ai.act_interval) {
//     change_state(ai, BehaviourState::IDLING);
// }
//}

// void OrganismAISystem::update_absorbing(ncore::EcsWorld &world, ncore::EcsEntity &entity, OrganismAIComponent &ai,
//                                         OrganismComponent &organism, double delta) {
//     absorb_food(world, entity, ai, organism);
//
//     ai.act_timer += delta;
//
//     if (!ai.is_food_found)
//         change_state(ai, BehaviourState::RUN_TUMBLE);
//     else if (ai.act_timer > ai.act_interval * 5.0f) {
//         ai.is_food_found = false;
//         change_state(ai, BehaviourState::EVALUATE);
//     }
// }

// void OrganismAISystem::update_eval(ncore::EcsEntity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
//                                    ncore::EcsWorld &world, double delta) {
//  ai.act_timer += delta;

// if (organism.cur_energy > organism.genome.energy_capacity * 0.7f && ai.reproduce_interval > 100.0f) {
//     ai.reproduced = false;
//     NC_LOG_INFO("Organism {} reproduced", organism.entity ? organism.entity->id : 0);
//     reproduce(world, entity, ai, organism);
//     change_state(ai, BehaviourState::RUN_TUMBLE);
// } else if (ai.act_timer > ai.act_interval * 2.0f) {
//     change_state(ai, BehaviourState::RUN_TUMBLE);
// }
//}
//
// ncore::Vec2 OrganismAISystem::get_rand_dir() {
//    float angle = ncore::Random::rand_float(0.0f, 2.0f * std::numbers::pi_v<float>);
//    return ncore::Vec2(std::cos(angle), std::sin(angle));
//}
//
// void OrganismAISystem::absorb_food(ncore::EcsWorld &world, ncore::EcsEntity &entity, OrganismAIComponent &ai,
//                                   OrganismComponent &organism) {
// if (ai.captured_food == nullptr) {
//     NC_LOG_ERROR("Nutrient is not found while trying to absorb it!");
//     ai.is_food_found = false;
//     return;
// }

// auto &transform = world.get<ncore::TransformComponent>(entity);

// ai.captured_food->caught = true;
// ai.captured_food->eater_pos = transform.position;

// if (organism.cur_energy < organism.genome.energy_capacity) {
//     if (ai.captured_food->cur_energy > 0.0f)
//         organism.cur_energy += ai.absorb_speed;

//    ai.captured_food->cur_energy -= ai.absorb_speed;
//    organism.fitness += 0.05f;
//}
//}
//
// void OrganismAISystem::reproduce(ncore::EcsWorld &world, ncore::EcsEntity &entity, OrganismAIComponent &ai,
//                                 OrganismComponent &organism) {
// if (ai.reproduced)
//     return;

// ai.reproduced = true;
// ai.reproduce_interval = 0.0f;

// auto &reg = world.context<SpeciesRegistry>();
// auto *species = reg.get_species_by_id(organism.species_id);
// if (!species) {
//     NC_LOG_ERROR("Cannot reproduce: species not found for organism {}", organism.entity ? organism.entity->id :
//     0); return;
// }

// auto &offspring = OrganismFactory::create(world, reg, species);
// if (organism.entity)
//     NC_LOG_INFO("Organism {} has been birthed into the world", organism.entity->id);

// if (offspring.entity && world.has<ncore::TransformComponent>(entity)) {
//     auto &parent_tsfm = world.get<ncore::TransformComponent>(entity);
//     auto &offspring_tsfm = world.get<ncore::TransformComponent>(*offspring.entity);
//     offspring_tsfm.position = parent_tsfm.position;
// }

// if (offspring.entity && offspring.genome.mutate(5, 1))
//     NC_LOG_INFO("Mutation has occurred on organism {}", offspring.entity->id);

// organism.cur_energy -= 15.0f;
//}
