#pragma once

#include <ncore/kernel/structures.h>

namespace ncore {

enum class ShapeType {
    Box,
    Circle,
    Capsule,
    Polygon
};
enum class BodyType {
    Static,
    Dynamic,
    Kinematic
};

struct PhysicsShape {
    ShapeType type = ShapeType::Box;
    float density  = 1.0f;
    float friction = 0.5f;
};

struct PhysicsBody {
    BodyType type = BodyType::Dynamic;
    Vec2 initial_pos;
    float mass           = 1.0f;
    float linear_damping = 0.5f;
};

} // namespace ncore
