#pragma once

#include <ncore/core/vector.h>

namespace nc {

struct RigidbodyComponent {
    Vec2f velocity;
    Vec2f pending_force{ 0.0f, 0.0f };
    Vec2f pending_impulse{ 0.0f, 0.0f };
    float linear_damping = 2.5f;
    NSTRUCTV(
        RigidbodyComponent, NC_F( RigidbodyComponent, velocity ), NC_F( RigidbodyComponent, pending_force ),
        NC_F( RigidbodyComponent, pending_impulse ), NC_F( RigidbodyComponent, linear_damping )
    )
};

} // namespace nc
