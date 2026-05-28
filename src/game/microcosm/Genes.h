#pragma once

#include <random>
#include <stdint.h>

#include <SDL3/SDL_pixels.h>

class Genes {
public:
    Genes() {
        energyCapacity = getRandomValue(10, 100);
        speed = getRandomValue(1, 3);
        size = getRandomValue(10, 30);
        membraneColour = {static_cast<uint8_t>(getRandomValue(1, 255)), static_cast<uint8_t>(getRandomValue(1, 255)),
                          static_cast<uint8_t>(getRandomValue(1, 255)), 150};
        aggresiveness = getRandomValue(1, 20);
    }

    // Traits (Parameters) of the organisms
    float energyCapacity;
    float speed;
    float size;
    float aggresiveness;
    SDL_Color membraneColour;

    static float getRandomValue(const int min, const int max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);

        return dis(gen);
    }

    bool mutate(const uint32_t mutationProb, const float mutationRate) {
        //! \todo Fix mutation probability calculations
        if (static_cast<uint32_t>(getRandomValue(0, mutationProb * 2)) < mutationProb) {
            energyCapacity += getRandomValue(-mutationRate, mutationRate);
            speed += getRandomValue(-mutationRate, mutationRate);
            size += getRandomValue(-mutationRate, mutationRate);
            aggresiveness += getRandomValue(-mutationRate, mutationRate);
            return true;
        }

        return false;
    }
};
