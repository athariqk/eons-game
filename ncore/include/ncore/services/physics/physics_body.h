#pragma once

#include <ncore/core/vector.h>

namespace nc {

enum class ShapeType {
    BOX,
    CIRCLE,
    CAPSULE,
    POLYGON
};
enum class BodyType {
    STATIC,
    DYNAMIC,
    KINEMATIC
};

struct PhysicsShape {
    ShapeType type = ShapeType::BOX;
    float density  = 1.0f;
    float friction = 0.5f;
};

struct PhysicsBody {
    BodyType type = BodyType::DYNAMIC;
    Vec2 initial_pos;
    float mass           = 1.0f;
    float linear_damping = 0.5f;
};

} // namespace nc
