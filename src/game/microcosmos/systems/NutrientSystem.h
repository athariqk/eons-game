#pragma once

#include <System.h>

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
    NutrientSystem() {
        SetPriority(55);
    }

    void OnFixedUpdate(Aeon::World &world, double fixedDelta) override;
};
