#pragma once

#include <System.h>

namespace ncore {
class World;
}

/**
 * @brief System that manages species lifecycle
 * 
 * Handles species extinction when all organisms die
 */
class SpeciesSystem : public ncore::System {
public:
    SpeciesSystem() {
        set_priority(50);  // Run early
    }

    void on_fixed_update(ncore::World &world, double fixedDelta) override;
};
