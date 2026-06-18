#pragma once

#include <ncore/kernel/structures.h>

class FoodComponent {

public:
    FoodComponent() : cur_energy(0.0f) {}
    explicit FoodComponent(const float energy) : cur_energy(energy) {}

    float cur_energy;
    bool caught = false;
    ncore::Vec2 eater_pos;
};
