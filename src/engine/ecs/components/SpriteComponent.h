#pragma once

#include <Component.h>

namespace ncore {

/**
 * @brief Component that stores an entity's sprite resources
 */
struct SpriteComponent : public Component {
    SpriteComponent(std::string texturePath) : texture_path(texturePath) {}
    std::string texture_path;
    Rect rect{};
    void *texture_ptr = nullptr;
    float angle = 0; // in radians
};

/**
 * @brief Temporary component for rendering simple circles
 */
struct TempCircleComponent : public Component {
    float radius = 1.0f;
    Color color{0, 0, 0, 255};
    bool filled = false;
    bool edge = false;
};

} // namespace ncore
