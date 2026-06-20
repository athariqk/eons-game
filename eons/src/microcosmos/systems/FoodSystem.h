#pragma once

#include <ncore/kernel/structures.h>
#include <ncore/runtime/ecs/ecs_system.h>

class FoodSystem : public ncore::EcsSystem {
public:
    FoodSystem() { set_priority(55); }

    void on_init(ncore::EcsWorld &world) override;
    void on_fixed_update(ncore::EcsWorld &world, double fixed_delta) override;

    void handle_nutrient_spawns(ncore::EcsWorld &world, int amountToSpawn);

private:
    int max_n_foods = 2000;
    float spawn_interval = 10;
    ncore::Vec2 spawn_area{200.0f, 200.0f};
    float spawn_timer = 0.0f;
};
