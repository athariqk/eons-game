#pragma once

#include <System.h>

class MicrocosmWorld;

namespace ncore {
class World;
}

/**
 * @brief System that updates organism state
 *
 * Handles energy depletion, fitness updates, and death conditions
 */
class OrganismSystem : public ncore::System {
public:
    OrganismSystem() { set_priority(55); }

    void on_fixed_update(ncore::World &world, double fixedDelta) override;
};
