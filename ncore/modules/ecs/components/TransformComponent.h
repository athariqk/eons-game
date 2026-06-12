#pragma once

#include <modules/ecs/Component.h>
#include <modules/utils/Structures.h>

namespace ncore {

/**
 * @brief Component that stores an entity's position, angle, and scale
 *
 * This is a fundamental component that most entities will have.
 * It represents the entity's transform in 2D space.
 */
struct TransformComponent : public Component {
    TransformComponent() = default;
    TransformComponent(const Vec2 &pos) : position(pos) {}
    TransformComponent(const Vec2 &pos, float rot) : position(pos), angle(rot) {}
    TransformComponent(const Vec2 &pos, float rot, const Vec2 &dim) : position(pos), angle(rot), dimension(dim) {}

    Vec2 position{0.0f, 0.0f};
    float angle = 0.0f; // in radians
    Vec2 dimension{0.0f, 0.0f};
    float scale = 1.0f;

    std::string to_string() const {
        return std::format("TransformComponent<position={}, angle={}, dimension={}, scale={}>", position.to_string(),
                           angle, dimension.to_string(), scale);
    }
};

} // namespace ncore
