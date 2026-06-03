#pragma once

#include <box2d/box2d.h>

#include <Component.h>
#include <Vector2D.h>

namespace Aeon {

struct Vector2D;

/**
 * @brief Component that stores an entity's rigidbody properties
 */
struct RigidBodyComponent : public Component {
    b2BodyId b2Id{};
    Vector2D velocity;
    float mass;
    float linearDamping = 0.5f;

    Vector2D pendingForce{0.0f, 0.0f};
    Vector2D pendingImpulse{0.0f, 0.0f};
};

} // namespace Aeon
