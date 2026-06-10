#pragma once

#include <vector>

#include <System.h>
#include <Vec2D.h>

namespace ncore {
class World;
}

/**
 * @brief System that updates food state
 *
 * Handles food destruction when depleted
 */
class FoodSystem : public ncore::System {
public:
    FoodSystem() { set_priority(55); }

	bool on_init(ncore::World &world) override;
    void on_fixed_update(ncore::World &world, double fixedDelta) override;

    //! \brief Distributes food spawning across the environment (spawn area)
    void handle_nutrient_spawns(ncore::World &world, int amountToSpawn);

private:
    int max_n_foods = 2000;
    float spawn_interval = 10; // in seconds
    ncore::Vec2D spawn_area{200.0f, 200.0f};
    float spawn_timer = 0.0f;
};
