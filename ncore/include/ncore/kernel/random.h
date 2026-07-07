#pragma once

#include <random>

namespace nc {

/**
 * @brief PRNG
 */
class Random {
public:
    /**
     * Returns a random float between min and max.
     * @param min lower bound
     * @param max upper bound
     * @return random value
     */
    static float rand_float( float min, float max )
    {
        static std::random_device rd;
        static std::mt19937 gen( rd() );
        std::uniform_real_distribution<float> dist( min, max );
        return dist( gen );
    }

    /**
     * Returns a random int between min and max.
     * @param min lower bound
     * @param max upper bound
     * @return random value
     */
    static int rand_int( int min, int max )
    {
        static std::random_device rd;
        static std::mt19937 gen( rd() );
        std::uniform_int_distribution<int> dist( min, max );
        return dist( gen );
    }
};

} // namespace nc
