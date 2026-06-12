#pragma once

#include <modules/ecs/Component.h>
#include <modules/resources/ResourceHandle.h>
#include <modules/utils/Structures.h>

namespace ncore {

/**
 * @brief Component that stores an entity's sprite resources
 */
struct SpriteComponent : public Component {
    SpriteComponent(std::string p_filepath) : filepath(p_filepath) {}
    std::string filepath;
    Vec4 rect{};
    ResourceHandle res;
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
