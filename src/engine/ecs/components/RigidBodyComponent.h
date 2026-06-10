#pragma once

#include <box2d/box2d.h>

#include <Component.h>
#include <Vec2D.h>

namespace ncore {

struct Vec2D;

/**
 * @brief Component that stores an entity's rigidbody properties
 */
struct RigidBodyComponent : public Component {
    b2BodyId b2Id{};
    float mass;
    float linear_damping = 0.5f;
    Vec2D velocity;
    Vec2D pending_force{0.0f, 0.0f};
    Vec2D pending_impulse{0.0f, 0.0f};
};

} // namespace ncore
