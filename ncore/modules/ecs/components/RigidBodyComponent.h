#pragma once

#include <box2d/box2d.h>

#include <modules/ecs/Component.h>
#include <utils/Structures.h>

namespace ncore {

struct Vec2;

/**
 * @brief Component that stores an entity's rigidbody properties
 */
struct RigidBodyComponent : public Component {
    b2BodyId b2Id{};
    float mass;
    float linear_damping = 0.5f;
    Vec2 velocity;
    Vec2 pending_force{0.0f, 0.0f};
    Vec2 pending_impulse{0.0f, 0.0f};
};

} // namespace ncore
