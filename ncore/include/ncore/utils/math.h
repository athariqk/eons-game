#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

namespace nc::math {

constexpr float PI                = std::numbers::pi_v<float>;
constexpr float PI_OVER_HALF_CIRC = PI / 180;
constexpr float HALF_CIRC_OVER_PI = 180 / PI;
constexpr float HALF_PI           = PI / 2;
constexpr float DOUBLE_PI         = 2.0f * PI;
constexpr size_t KILOBYTE         = 1000;

/**
 * @brief Test if a ~= b within a floating-point epsilon error.
 */
static bool is_equal_approx( float a, float b )
{
    return std::fabs( a - b ) <= std::numeric_limits<float>::epsilon() * std::fmax( std::fabs( a ), std::fabs( b ) );
}

/**
 * @brief Convert degrees to radians: d * (pi / 180).
 */
static float deg_to_rad( float degrees )
{
    return degrees * PI_OVER_HALF_CIRC;
}

/**
 * @brief Convert radians to degrees: r * (180 / pi).
 */
static float rad_to_deg( float radians )
{
    return radians * HALF_CIRC_OVER_PI;
}

static size_t bytes_to_kb( size_t value )
{
    return value / KILOBYTE;
}

} // namespace nc::math
