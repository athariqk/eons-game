#pragma once

#include "Entity.h"
#include "Vector2D.h"

/**
 * @brief Component representing a nutrient in the simulation
 */
class NutrientComponent : public Aeon::Component {
public:
    explicit NutrientComponent(const float energy) : curEnergy(energy) {}
    ~NutrientComponent() override {}

    float curEnergy;
    bool caught = false;
    Aeon::Vector2D organismPos;
};
