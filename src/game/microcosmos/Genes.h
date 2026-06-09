#pragma once

#include <stdint.h>

#include <Color.h>
#include <Random.h>

struct Genes {
public:
    Genes() {
        energyCapacity = Aeon::Random::RandomInt(50, 100);
        speed = Aeon::Random::RandomFloat(3.0f, 10.0f); // Speed in Meters per second (e.g., 3.0m/s to 10.0m/s)
        size = Aeon::Random::RandomFloat(0.3f, 1.0f); // Size in Meters (e.g., 0.3m to 1.0m diameter)
        membraneColour = Color(static_cast<uint8_t>(Aeon::Random::RandomInt(1, 255)),
                               static_cast<uint8_t>(Aeon::Random::RandomInt(1, 255)),
                               static_cast<uint8_t>(Aeon::Random::RandomInt(1, 255)), 150);
        aggresiveness = Aeon::Random::RandomInt(1, 20);
    }

    // Traits (Parameters) of the organisms
    float energyCapacity;
    float speed;
    float size;
    float aggresiveness;
    Color membraneColour;

    bool mutate(const uint32_t mutationProb, const float mutationRate) {
        //! \todo Fix mutation probability calculations
        if (static_cast<uint32_t>(Aeon::Random::RandomInt(0, mutationProb * 2)) < mutationProb) {
            energyCapacity += Aeon::Random::RandomInt(-mutationRate, mutationRate);
            speed += Aeon::Random::RandomInt(-mutationRate, mutationRate);
            size += Aeon::Random::RandomInt(-mutationRate, mutationRate);
            aggresiveness += Aeon::Random::RandomInt(-mutationRate, mutationRate);
            return true;
        }

        return false;
    }
};
