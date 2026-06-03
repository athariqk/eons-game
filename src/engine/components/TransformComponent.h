#pragma once

#include <Component.h>
#include <Vector2D.h>

namespace Aeon {

/**
 * @brief Component that stores an entity's position, rotation, and scale
 *
 * This is a fundamental component that most entities will have.
 * It represents the entity's transform in 2D space.
 */
struct TransformComponent : public Component {
    Vector2D position{0.0f, 0.0f};
    float rotation = 0.0f; // in radians
    Vector2D dimension{0.0f, 0.0f};
    float scale = 1.0f;

    TransformComponent() = default;

    TransformComponent(const Vector2D &pos) : position(pos) {}

    TransformComponent(const Vector2D &pos, float rot) : position(pos), rotation(rot) {}

    TransformComponent(const Vector2D &pos, float rot, const Vector2D &dim) :
        position(pos), rotation(rot), dimension(dim) {}

    std::string ToString() const {
        return std::format("TransformComponent<position={}, rotation={}, dimension={}, scale={}>", position.ToString(),
                           rotation, dimension.ToString(), scale);
    }
};

} // namespace Aeon
