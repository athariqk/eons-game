#pragma once

#include <algorithm>

namespace nc::math {

static float kPI_OVER_180 = 0.01745329252f;
static float k180_OVER_PI = 57.2957795131f;

static bool is_equal_approx( float a, float b )
{
    return std::fabs( a - b ) <= std::numeric_limits<float>::epsilon() * std::fmax( std::fabs( a ), std::fabs( b ) );
}

static float deg_to_rad( float degrees )
{
    return degrees * kPI_OVER_180;
}

static float rad_to_deg( float radians )
{
    return radians * k180_OVER_PI;
}

} // namespace nc::math
