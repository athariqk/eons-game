#include "FoodSystem.h"

#include <ncore/kernel/random.h>
#include <ncore/modules/ecs/ecs_world.h>

#include <microcosmos/GameGroups.h>
#include <microcosmos/components/FoodComponent.h>

// void FoodSystem::on_init(ncore::EcsWorld &world) {}

// void FoodSystem::on_process(ncore::EcsWorld &world, double delta_time) {
//  int count = 0;
//  for (const auto *entity_ptr : world.get_group(GameGroups::NUTRIENTS_GROUP)) {
//      if (!entity_ptr->is_enabled)
//          continue;

//    if (world.has<FoodComponent>(*entity_ptr)) {
//        auto &nutrient = world.get<FoodComponent>(*entity_ptr);

//        if (nutrient.cur_energy < 0)
//            world.destroy(*const_cast<ncore::EcsEntity *>(entity_ptr));
//    }

//    count++;
//}

// auto spawn_interval_factor = ncore::Random::rand_float(0.5f, 1.0f);
// spawn_timer += delta_time;
// if (spawn_timer >= spawn_interval * spawn_interval_factor) {
//     spawn_timer = 0.0f;
//     auto spawn_factor = ncore::Random::rand_float(0.03f, 0.07f);
//     int amount_to_spawn = 0;
//     while (count + amount_to_spawn < max_n_foods && amount_to_spawn < max_n_foods * spawn_factor)
//         amount_to_spawn++;
//     handle_nutrient_spawns(world, amount_to_spawn);
// }
//}

// void FoodSystem::handle_nutrient_spawns(ncore::EcsWorld &world, int amountToSpawn) {
//  for (int i = 0; i < amountToSpawn; i++) {
//      auto &entity = world.create_entity();

//    float size = ncore::Random::rand_float(0.2f, 0.6f);

//    float spawn_x = ncore::Random::rand_float(-spawn_area.x, spawn_area.x);
//    float spawn_y = ncore::Random::rand_float(-spawn_area.y, spawn_area.y);

//    world.emplace<ncore::TransformComponent>(entity, ncore::Vec2(spawn_x, spawn_y), 0.0f, ncore::Vec2(size,
//    size)); world.emplace<ncore::RigidbodyComponent>(entity); auto &food =
//    world.emplace<FoodComponent>(entity, 5.0f); food.entity = &entity;
//    world.emplace<ncore::SpriteComponent>(entity, std::string("assets/nutrient.webp"));
//    world.add_group(entity, GameGroups::NUTRIENTS_GROUP);
//}

// NC_LOG_INFO("Spawned {} nutrients to the environment", amountToSpawn);
//}
