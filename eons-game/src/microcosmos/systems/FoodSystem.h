#pragma once

#include <ncore/kernel/structures.h>
#include <ncore/runtime/ecs_system.h>

// class FoodSystem : public nc::EcsSystem {
//     NCLASS(FoodSystem, nc::EcsSystem)
//
// public:
//     FoodSystem() { set_priority(55); }
//
//     void on_init(nc::EcsWorld &world) override;
//     void on_process(nc::EcsWorld &world, double delta_time) override;
//
//     void handle_nutrient_spawns(nc::EcsWorld &world, int amountToSpawn);
//
// private:
//     int max_n_foods = 2000;
//     float spawn_interval = 10;
//     nc::Vec2 spawn_area{200.0f, 200.0f};
//     float spawn_timer = 0.0f;
// };
