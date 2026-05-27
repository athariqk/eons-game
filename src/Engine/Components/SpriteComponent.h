#pragma once

#include <SDL3/SDL_rect.h>

#include "EntitySystem.h"

class TransformComponent;
struct SDL_Texture;

class SpriteComponent : public Component {
public:
    SpriteComponent() = default;
    explicit SpriteComponent(const char *path);
    ~SpriteComponent() override;

    void OnInit() override;
    void OnUpdate(float delta) override;
    void OnDraw() override;

private:
    TransformComponent *transform{};
    SDL_Texture *texture{};
    SDL_FRect destRect{};
};
