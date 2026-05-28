#include "SpriteComponent.h"

#include <SDL3/SDL_render.h>

#include <IGraphicsContext.h>
#include <Scene.h>
#include <Viewport.h>
#include "TextureManager.h"
#include "TransformComponent.h"
#include "Vector2D.h"

SpriteComponent::SpriteComponent(const char *path) : m_texturePath(path) {
    // Texture loading will happen in OnInit when entity is set up
}

SpriteComponent::~SpriteComponent() {
    if (texture) {
        SDL_DestroyTexture(texture);
    }
}

void SpriteComponent::OnInit() {
    transform = &entity->GetComponent<TransformComponent>();

    destRect.w = transform->width * transform->scale;
    destRect.h = transform->height * transform->scale;

    // Load texture if we have a path
    if (!m_texturePath.empty()) {
        Scene *scene = entity->GetManager().GetScene();
        auto ctx = scene->GetViewport()->GetGraphicsContext();
        if (scene && ctx) {
            // Get SDL renderer from graphics context for texture loading
            SDL_Renderer *renderer = static_cast<SDL_Renderer *>(ctx->GetNativeHandle());
            if (renderer) {
                texture = TextureManager::LoadTexture(renderer, m_texturePath.c_str());
            }
        }
    }
}

void SpriteComponent::OnUpdate(float delta) {
    destRect.w = transform->width * transform->scale;
    destRect.h = transform->height * transform->scale;
}

void SpriteComponent::OnDraw() {
    if (!texture)
        return;

    Scene *scene = entity->GetManager().GetScene();
    auto ctx = scene->GetViewport()->GetGraphicsContext();
    if (scene && ctx) {
        SDL_Renderer *renderer = static_cast<SDL_Renderer *>(ctx->GetNativeHandle());
        if (renderer) {
            // Transform world position to screen position using viewport
            Viewport2D *viewport = scene->GetViewport();
            Vector2D screenPos = transform->position;

            if (viewport) {
                screenPos = viewport->WorldToScreen(transform->position);
            }

            destRect.x = screenPos.x - (destRect.w / 2);
            destRect.y = screenPos.y - (destRect.h / 2);

            TextureManager::Draw(renderer, texture, nullptr, &destRect);
        }
    }
}
