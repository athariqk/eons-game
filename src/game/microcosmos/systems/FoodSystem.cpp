#include "FoodSystem.h"

#include <Entity.h>
#include <Logger.h>
#include <Random.h>
#include <RigidBodyComponent.h>
#include <Services.h>
#include <SpriteComponent.h>
#include <TransformComponent.h>
#include <Viewport.h>
#include <World.h>

#include "MicrocosmWorld.h"
#include "components/FoodComponent.h"

bool FoodSystem::on_init(ncore::World &world) { return true; }

void FoodSystem::on_fixed_update(ncore::World &world, double fixedDelta) {
    auto *viewport = world.get_main_loop().get_services().try_get<ncore::Viewport2D>();

    int count = 0;
    for (const auto &entity_ptr: world.get_group(MicrocosmWorld::GroupLabels::NUTRIENTS_GROUP)) {
        if (!entity_ptr->is_enabled())
            continue;

        if (entity_ptr->has_component<FoodComponent>()) {
            auto &nutrient = entity_ptr->get_component<FoodComponent>();

            // Destroy if depleted
            if (nutrient.cur_energy < 0) {
                entity_ptr->destroy();
            }
        }

        count++;
    }

    auto spawn_interval_factor = ncore::Random::rand_float(0.5f, 1.0f);
    spawn_timer += fixedDelta;
    if (spawn_timer >= spawn_interval * spawn_interval_factor) {
        spawn_timer = 0.0f;
        auto spawn_factor = ncore::Random::rand_float(0.03f, 0.07f);
        int amount_to_spawn = 0;
        while (count + amount_to_spawn < max_n_foods && amount_to_spawn < max_n_foods * spawn_factor) {
            amount_to_spawn++;
        }
        handle_nutrient_spawns(world, amount_to_spawn);
    }
}

void FoodSystem::handle_nutrient_spawns(ncore::World &world, int amountToSpawn) {
    for (int i = 0; i < amountToSpawn; i++) {
        auto &nutrient(world.create_entity());

        float size = ncore::Random::rand_float(0.2f, 0.6f);

        float spawn_x = ncore::Random::rand_float(-spawn_area.x, spawn_area.x);
        float spawn_y = ncore::Random::rand_float(-spawn_area.y, spawn_area.y);

        nutrient.add_component<ncore::TransformComponent>(ncore::Vec2D(spawn_x, spawn_y), 0.0f,
                                                          ncore::Vec2D(size, size));
        nutrient.add_component<ncore::RigidBodyComponent>();
        nutrient.add_component<FoodComponent>(5.0f);
        nutrient.add_component<ncore::SpriteComponent>("assets/nutrient.webp");
        nutrient.add_group(MicrocosmWorld::GroupLabels::NUTRIENTS_GROUP);
    }

    LOG_INFO(ncore::log::GAME, "Spawned {} nutrients to the environment", amountToSpawn);
}
