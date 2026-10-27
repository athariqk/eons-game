#pragma once

#include "vector.h"

namespace nc {

struct REFLECT NCAPI AABB {
    REFLECT Vec3f min = { 0.0f, 0.0f, 0.0f };
    REFLECT Vec3f max = { 0.0f, 0.0f, 0.0f };

    AABB() = default;
    AABB( const Vec3f& p_min, const Vec3f& p_max ) : min( p_min ), max( p_max ) {}
};

} // namespace nc
