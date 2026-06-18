#pragma once

#include <ncore/kernel/types.h>

namespace ncore {

struct Vec2;

struct EcsRigidbody {
    Vec2 velocity;
    Vec2 pending_force{0.0f, 0.0f};
    Vec2 pending_impulse{0.0f, 0.0f};
    float linear_damping = 2.5f;
};

} // namespace ncore
