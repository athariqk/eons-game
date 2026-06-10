#pragma once

#include "Entity.h"
#include "Vec2D.h"

/**
 * @brief Component representing a food source in the simulation
 */
class FoodComponent : public ncore::Component {
public:
    explicit FoodComponent(const float energy) : cur_energy(energy) {}
    ~FoodComponent() override {}

    float cur_energy;
    bool caught = false;
    ncore::Vec2D eater_pos;
};
