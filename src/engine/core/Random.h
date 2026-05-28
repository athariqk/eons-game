//
// Created by athariqk on 04/03/2025.
//
#pragma once
#ifndef RANDOM_H
#define RANDOM_H

#include <random>

class Random {
public:
    /**
     * Returns a random float between min and max.
     * @param min lower bound
     * @param max upper bound
     * @return random value
     */
    static float RandomFloat(float min, float max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

    /**
     * Returns a random int between min and max.
     * @param min lower bound
     * @param max upper bound
     * @return random value
     */
    static int RandomInt(int min, int max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen);
    }
};

#endif // RANDOM_H
