#pragma once

#include <Component.h>

namespace Aeon {

/**
 * @brief Component that stores an entity's sprite resources
 */
struct SpriteComponent : public Component {
    SpriteComponent(std::string texturePath) : texturePath(texturePath) {}

    std::string texturePath;
    SDL_Texture *texturePtr = nullptr;
    SDL_FRect rect{};
};

/**
 * @brief Temporary component for rendering simple circles
 */
struct TempCircleComponent : public Component {
    float radius = 1.0f;
    SDL_Color color{0, 0, 0, 255};
    bool filled = false;
    bool edge = false;
};

} // namespace Aeon
