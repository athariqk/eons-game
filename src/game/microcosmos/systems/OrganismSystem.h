#pragma once

#include <System.h>

class MicrocosmWorld;

namespace Aeon {
class World;
}

/**
 * @brief System that updates organism state
 *
 * Handles energy depletion, fitness updates, and death conditions
 */
class OrganismSystem : public Aeon::System {
public:
    OrganismSystem() { SetPriority(55); }

    void OnFixedUpdate(Aeon::World &world, double fixedDelta) override;
};
