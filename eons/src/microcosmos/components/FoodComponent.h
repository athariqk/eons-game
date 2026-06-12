#pragma once

#include "modules/ecs/Entity.h"
#include "utils/Structures.h"

/**
 * @brief Component representing a food source in the simulation
 */
class FoodComponent : public ncore::Component {
public:
    explicit FoodComponent(const float energy) : cur_energy(energy) {}
    ~FoodComponent() override {}

    float cur_energy;
    bool caught = false;
    ncore::Vec2 eater_pos;
};
