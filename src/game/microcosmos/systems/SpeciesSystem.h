#pragma once

#include <System.h>

namespace Aeon {
class World;
}

/**
 * @brief System that manages species lifecycle
 * 
 * Handles species extinction when all organisms die
 */
class SpeciesSystem : public Aeon::System {
public:
    SpeciesSystem() {
        SetPriority(50);  // Run early
    }

    void OnFixedUpdate(Aeon::World &world, double fixedDelta) override;
};
