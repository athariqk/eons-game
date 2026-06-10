#pragma once

#include <vector>

#include <System.h>
#include <Vector2D.h>

namespace Aeon {
class World;
}

/**
 * @brief System that updates nutrient state
 *
 * Handles nutrient destruction when depleted
 */
class NutrientSystem : public Aeon::System {
public:
    NutrientSystem() { SetPriority(55); }

	bool OnInit(Aeon::World &world) override;
    void OnFixedUpdate(Aeon::World &world, double fixedDelta) override;

    //! \brief Distributes food spawning across the environment (spawn area)
    void HandleSpawnNutrients(Aeon::World &world, int amountToSpawn);

private:
    int m_maxAmountOfNutrients = 2000;
    float m_spawnInterval = 10; // in seconds
    Aeon::Vector2D m_spawnArea{200.0f, 200.0f};

    float m_spawnTimer = 0.0f;
};
