#pragma once

#include <stdint.h>

#include <utils/Color.h>
#include <utils/Random.h>

struct Genes {
public:
    Genes() {
        energy_capacity = ncore::Random::rand_int(50, 100);
        speed = ncore::Random::rand_float(3.0f, 10.0f); // Speed in Meters per second (e.g., 3.0m/s to 10.0m/s)
        size = ncore::Random::rand_float(0.3f, 1.0f); // Size in Meters (e.g., 0.3m to 1.0m diameter)
        membrane_color = Color(static_cast<uint8_t>(ncore::Random::rand_int(1, 255)),
                               static_cast<uint8_t>(ncore::Random::rand_int(1, 255)),
                               static_cast<uint8_t>(ncore::Random::rand_int(1, 255)), 150);
        aggresiveness = ncore::Random::rand_int(1, 20);
    }

    // Traits (Parameters) of the organisms
    float energy_capacity;
    float speed;
    float size;
    float aggresiveness;
    Color membrane_color;

    bool mutate(const uint32_t p_mut_prob, const float mut_rate) {
        //! \todo Fix mutation probability calculations
        if (static_cast<uint32_t>(ncore::Random::rand_int(0, p_mut_prob * 2)) < p_mut_prob) {
            energy_capacity += ncore::Random::rand_int(-mut_rate, mut_rate);
            speed += ncore::Random::rand_int(-mut_rate, mut_rate);
            size += ncore::Random::rand_int(-mut_rate, mut_rate);
            aggresiveness += ncore::Random::rand_int(-mut_rate, mut_rate);
            return true;
        }

        return false;
    }
};
