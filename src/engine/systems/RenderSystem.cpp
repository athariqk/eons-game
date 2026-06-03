#include "RenderSystem.h"

#include <Logger.h>
#include <PrimitiveShape.h>
#include <RigidBodyComponent.h>
#include <SpriteComponent.h>
#include <TextureManager.h>
#include <TransformComponent.h>
#include <World.h>

namespace Aeon {

bool RenderSystem::OnInit(World &world) {
    m_viewport = world.GetMainLoop().GetServices().TryGet<Viewport2D>();
    if (!m_viewport) {
        LOG_ERROR("Viewport2D service not found!");
        return false;
    }

    m_graphics = m_viewport->GetGraphicsContext();
    if (!m_graphics) {
        LOG_ERROR("Graphics context not found in Viewport2D!");
        return false;
    }

    LOG_INFO("RenderSystem initialized");

    return true;
}

void RenderSystem::OnRender(World &world, IGraphicsContext &graphics) {
    auto renderer = static_cast<SDL_Renderer *>(graphics.GetNativeHandle());
    if (!renderer) {
        LOG_ERROR("Graphics renderer not initialized!");
        return;
    }

    for (const auto &entityPtr: world.GetEntities()) {
        if (!entityPtr->IsEnabled())
            continue;

        if (entityPtr->HasComponent<SpriteComponent>()) {
            RenderSprite(entityPtr.get(), renderer);
        }

        if (entityPtr->HasComponent<TempCircleComponent>()) {
            RenderTempCircle(entityPtr.get(), renderer);
        }

        if (entityPtr->HasComponent<RigidBodyComponent>()) {
            RenderPhysicsDebug(entityPtr.get(), renderer);
        }
    }
}

void RenderSystem::RenderSprite(Entity *entityPtr, SDL_Renderer *renderer) {
    if (!entityPtr->HasComponent<TransformComponent>()) {
        LOG_ERROR("Entity {} has no TransformComponent to render with!", entityPtr->GetID());
        return;
    }

    auto &transform = entityPtr->GetComponent<TransformComponent>();
    auto &sprite = entityPtr->GetComponent<SpriteComponent>();

    if (!sprite.texturePtr) {
        sprite.texturePtr = TextureManager::LoadTexture(renderer, sprite.texturePath.c_str());
        if (!sprite.texturePtr) {
            return;
        }
        // Set sprite rect size based on transform dimensions if texture loaded successfully
        sprite.rect.w = transform.dimension.x * transform.scale;
        sprite.rect.h = transform.dimension.y * transform.scale;
    }

    float zoom = m_viewport->GetMainCamera()->GetZoom();

    float scaledWidth = transform.dimension.x * transform.scale * m_viewport->GetPixelsPerMeter() * zoom;
    float scaledHeight = transform.dimension.y * transform.scale * m_viewport->GetPixelsPerMeter() * zoom;

    Vector2D screenPos = m_viewport->WorldToScreen(transform.position);

    // Apply the scaled dimensions to the target drawing rectangle
    sprite.rect.w = static_cast<int>(scaledWidth);
    sprite.rect.h = static_cast<int>(scaledHeight);
    sprite.rect.x = static_cast<int>(screenPos.x - (scaledWidth / 2.0f));
    sprite.rect.y = static_cast<int>(screenPos.y - (scaledHeight / 2.0f));

    TextureManager::Draw(renderer, sprite.texturePtr, nullptr, &sprite.rect);
}

void RenderSystem::RenderTempCircle(Entity *entityPtr, SDL_Renderer *renderer) {
    if (!entityPtr->HasComponent<TransformComponent>()) {
        LOG_ERROR("Entity {} has no TransformComponent to render with!", entityPtr->GetID());
        return;
    }

    auto &transform = entityPtr->GetComponent<TransformComponent>();
    auto &circle = entityPtr->GetComponent<TempCircleComponent>();

    float zoom = m_viewport->GetMainCamera()->GetZoom();
    float scaledRadius = circle.radius * m_viewport->GetPixelsPerMeter() * zoom;

    Vector2D screenPos = m_viewport->WorldToScreen(transform.position);
    PrimitiveShape::DrawCircle(renderer, screenPos.x, screenPos.y, scaledRadius, circle.color, circle.filled,
                               circle.edge);
}

void RenderSystem::RenderPhysicsDebug(Entity *entityPtr, SDL_Renderer *renderer) {
    if (!entityPtr->HasComponent<TransformComponent>()) {
        LOG_ERROR("Entity {} has no TransformComponent to render with!", entityPtr->GetID());
        return;
    }

    auto &transform = entityPtr->GetComponent<TransformComponent>();
    auto &body = entityPtr->GetComponent<RigidBodyComponent>();
    auto [vx, vy] = body.velocity;

    float zoom = m_viewport->GetMainCamera()->GetZoom();
    Vector2D screenPos = m_viewport->WorldToScreen(transform.position);

    // Scale the debug velocity vectors and boxes by camera zoom
    float scaledWidth = transform.dimension.x * m_viewport->GetPixelsPerMeter() * zoom;
    float scaledVx = body.velocity.x * m_viewport->GetPixelsPerMeter() * zoom;
    float scaledVy = body.velocity.y * m_viewport->GetPixelsPerMeter() * zoom;

    PrimitiveShape::DrawLine(renderer, screenPos.x, screenPos.y, screenPos.x + scaledVx, screenPos.y + scaledVy,
                             {255, 0, 0, 255});
    PrimitiveShape::DrawRectangle(renderer, screenPos.x - (scaledWidth * 0.5f), screenPos.y - (scaledWidth * 0.5f),
                                  scaledWidth);
}

} // namespace Aeon
