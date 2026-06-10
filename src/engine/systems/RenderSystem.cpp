#include "RenderSystem.h"

#include <Logger.h>
#include <RigidBodyComponent.h>
#include <SpriteComponent.h>
#include <TextureManager.h>
#include <TransformComponent.h>
#include <World.h>
#include <cmath>
#include <imgui.h>

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

    m_textureManager = world.GetMainLoop().GetServices().TryGet<TextureManager>();
    if (!m_textureManager) {
        LOG_ERROR("TextureManager not found!");
        return false;
    }

    m_physics = world.GetMainLoop().GetServices().TryGet<Physics2D>();
    if (!m_physics) {
        LOG_ERROR("Physics2D service not found!");
        return false;
    }

    m_debugDrawCtx.renderer = m_graphics;
    m_debugDrawCtx.viewport = m_viewport;

    auto &fnc = m_physics->GetDebugDrawFnc();
    fnc.context = &m_debugDrawCtx;
    fnc.DrawPolygon = OnDebugDrawPolygon;
    fnc.DrawSolidPolygon = OnDebugDrawSolidPolygon;
    fnc.DrawCircle = OnDebugDrawCircle;
    fnc.DrawSolidCircle = OnDebugDrawSolidCircle;
    fnc.DrawSolidCapsule = OnDebugDrawSolidCapsule;
    fnc.DrawSegment = OnDebugDrawSegment;
    fnc.DrawTransform = OnDebugDrawTransform;
    fnc.DrawPoint = OnDebugDrawPoint;

    LOG_INFO("RenderSystem initialized");

    return true;
}

void RenderSystem::OnRender(World &world, IGraphicsContext &graphics) {
    m_physics->UpdateDebugDraw();

    for (const auto &entityPtr: world.GetEntities()) {
        if (!entityPtr->IsEnabled())
            continue;

        if (entityPtr->HasComponent<SpriteComponent>()) {
            RenderSprite(entityPtr.get());
        }

        if (entityPtr->HasComponent<TempCircleComponent>()) {
            RenderTempCircle(entityPtr.get());
        }

        if (m_physics->isDebugDraw && entityPtr->HasComponent<RigidBodyComponent>()) {
            RenderPhysicsDebug(entityPtr.get());
        }
    }
}

void RenderSystem::OnPostUpdate(World &world, double delta) {
    for (const auto &entityPtr: world.GetEntities()) {
        if (!entityPtr->IsEnabled() && entityPtr->HasComponent<SpriteComponent>()) {
            auto &sprite = entityPtr->GetComponent<SpriteComponent>();
            auto sdlTexture = static_cast<SDL_Texture *>(sprite.texturePtr);
            m_textureManager->DestroyTexture(sdlTexture);
        }
    }
}

void RenderSystem::OnGuiRender(World &world) {
    ImGui::Begin("Rendering");

    if (auto cam = m_viewport->GetMainCamera()) {
        const auto &camPos = cam->GetPosition();
        ImGui::Text("Camera position: (x: %f, y: %f)", camPos.x, camPos.y);

        float zoom = cam->GetZoom();
        ImGui::Text("Camera zoom: %f", zoom);
    }

    ImGui::End();
}

void RenderSystem::RenderSprite(Entity *entityPtr) {
    auto renderer = static_cast<SDL_Renderer *>(m_graphics->GetNativeHandle());
    if (!renderer) {
        LOG_ERROR("Graphics renderer not initialized!");
        return;
    }

    if (!entityPtr->HasComponent<TransformComponent>()) {
        LOG_ERROR("Entity {} has no TransformComponent to render with!", entityPtr->GetID());
        return;
    }

    auto &transform = entityPtr->GetComponent<TransformComponent>();
    auto &sprite = entityPtr->GetComponent<SpriteComponent>();

    if (!sprite.texturePtr) {
        sprite.texturePtr = m_textureManager->GetTexture(sprite.texturePath);
        if (!sprite.texturePtr) {
            return;
        }
        sprite.rect.w = transform.dimension.x * transform.scale;
        sprite.rect.h = transform.dimension.y * transform.scale;
    }

    float zoom = m_viewport->GetMainCamera()->GetZoom();

    float scaledWidth = transform.dimension.x * transform.scale * m_viewport->GetPixelsPerMeter() * zoom;
    float scaledHeight = transform.dimension.y * transform.scale * m_viewport->GetPixelsPerMeter() * zoom;
    float scaledAngle = transform.angle * transform.scale * m_viewport->GetPixelsPerMeter() * zoom;

    Vector2D screenPos = m_viewport->WorldToScreen(transform.position);

    sprite.rect.w = static_cast<int>(scaledWidth);
    sprite.rect.h = static_cast<int>(scaledHeight);
    sprite.rect.x = static_cast<int>(screenPos.x - (scaledWidth / 2.0f));
    sprite.rect.y = static_cast<int>(screenPos.y - (scaledHeight / 2.0f));

    auto sdlTexture = static_cast<SDL_Texture *>(sprite.texturePtr);
    auto sdlRect = SDL_FRect{sprite.rect.x, sprite.rect.y, sprite.rect.w, sprite.rect.h};
    TextureManager::Draw(renderer, sdlTexture, nullptr, &sdlRect, scaledAngle);
}

void RenderSystem::RenderTempCircle(Entity *entityPtr) {
    if (!entityPtr->HasComponent<TransformComponent>()) {
        LOG_ERROR("Entity {} has no TransformComponent to render with!", entityPtr->GetID());
        return;
    }

    auto &transform = entityPtr->GetComponent<TransformComponent>();
    auto &circle = entityPtr->GetComponent<TempCircleComponent>();

    float zoom = m_viewport->GetMainCamera()->GetZoom();
    float scaledRadius = circle.radius * m_viewport->GetPixelsPerMeter() * zoom;

    Vector2D screenPos = m_viewport->WorldToScreen(transform.position);
    m_graphics->DrawCircle(screenPos.x, screenPos.y, scaledRadius, circle.color, circle.filled, circle.edge);
}

/**
 * TODO: remove this and move ALL physics debug drawing in the Physics class
 */
void RenderSystem::RenderPhysicsDebug(Entity *entityPtr) {
    if (!entityPtr->HasComponent<TransformComponent>()) {
        LOG_ERROR("Entity {} has no TransformComponent to render with!", entityPtr->GetID());
        return;
    }

    auto &transform = entityPtr->GetComponent<TransformComponent>();
    auto &body = entityPtr->GetComponent<RigidBodyComponent>();
    auto [vx, vy] = body.velocity;

    float zoom = m_viewport->GetMainCamera()->GetZoom();
    Vector2D screenPos = m_viewport->WorldToScreen(transform.position);

    float scaledWidth = transform.dimension.x * m_viewport->GetPixelsPerMeter() * zoom;
    Vector2D scaledVel = body.velocity * m_viewport->GetPixelsPerMeter() * zoom;

    m_graphics->SetDrawColor(Color(255, 0, 0, 255));
    m_graphics->DrawLine(Rect{screenPos.x, screenPos.y, screenPos.x + scaledVel.x, screenPos.y + scaledVel.y});
    /*PrimitiveShape::DrawRectangle(renderer, screenPos.x - (scaledWidth * 0.5f), screenPos.y - (scaledWidth * 0.5f),
                                  scaledWidth);*/
}

// ---- DebugDrawFnc callbacks ----

void RenderSystem::OnDebugDrawPolygon(const Vector2D *vertices, int vertexCount, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer || vertexCount < 2)
        return;

    auto c = Color::Unpack(color, 255);
    dc->renderer->SetDrawColor(c);

    Vector2D screen[8];
    for (int i = 0; i < vertexCount; i++)
        screen[i] = dc->viewport->WorldToScreen(vertices[i]);

    for (int i = 0; i < vertexCount - 1; i++)
        dc->renderer->DrawLine(Rect{screen[i].x, screen[i].y, screen[i + 1].x, screen[i + 1].y});

    dc->renderer->DrawLine(Rect{screen[vertexCount - 1].x, screen[vertexCount - 1].y, screen[0].x, screen[0].y});
}

void RenderSystem::OnDebugDrawSolidPolygon(const Vector2D *vertices, int vertexCount, float radius, uint32_t color,
                                           void *context) {
    (void) radius;
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer || vertexCount < 3)
        return;

    Vector2D screen[8];
    for (int i = 0; i < vertexCount; i++)
        screen[i] = dc->viewport->WorldToScreen(vertices[i]);

    auto fillColor = Color::Unpack(color, 100);
    dc->renderer->FillConvexPolygon(screen, vertexCount, fillColor);

    auto outlineColor = Color::Unpack(color, 255);
    dc->renderer->SetDrawColor(outlineColor);

    for (int i = 0; i < vertexCount - 1; i++)
        dc->renderer->DrawLine(Rect{screen[i].x, screen[i].y, screen[i + 1].x, screen[i + 1].y});

    dc->renderer->DrawLine(Rect{screen[vertexCount - 1].x, screen[vertexCount - 1].y, screen[0].x, screen[0].y});
}

void RenderSystem::OnDebugDrawCircle(Vector2D center, float radius, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto screenPos = dc->viewport->WorldToScreen(center);
    float screenRadius = radius * dc->viewport->GetPixelsPerMeter() * dc->viewport->GetMainCamera()->GetZoom();

    auto c = Color::Unpack(color, 255);
    dc->renderer->DrawCircle(screenPos.x, screenPos.y, screenRadius, c, false, false);
}

void RenderSystem::OnDebugDrawSolidCircle(Vector2D center, float radius, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto screenPos = dc->viewport->WorldToScreen(center);
    float screenRadius = radius * dc->viewport->GetPixelsPerMeter() * dc->viewport->GetMainCamera()->GetZoom();

    auto fillC = Color::Unpack(color, 120);
    dc->renderer->DrawCircle(screenPos.x, screenPos.y, screenRadius, fillC, true, false);

    auto outlineC = Color::Unpack(color, 255);
    dc->renderer->DrawCircle(screenPos.x, screenPos.y, screenRadius, outlineC, false, true);
}

void RenderSystem::OnDebugDrawSolidCapsule(Vector2D p1, Vector2D p2, float radius, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto s1 = dc->viewport->WorldToScreen(p1);
    auto s2 = dc->viewport->WorldToScreen(p2);
    float ppm = dc->viewport->GetPixelsPerMeter() * dc->viewport->GetMainCamera()->GetZoom();
    float screenRadius = radius * ppm;

    auto fillC = Color::Unpack(color, 120);
    dc->renderer->DrawCircle(s1.x, s1.y, screenRadius, fillC, true, false);
    dc->renderer->DrawCircle(s2.x, s2.y, screenRadius, fillC, true, false);

    Vector2D dir = s2 - s1;
    float len = dir.Length();
    if (len < 0.001f)
        return;
    dir = dir * (1.0f / len);

    Vector2D perp(-dir.y, dir.x);
    Vector2D rectCorners[4] = {
        s1 + perp * screenRadius,
        s1 - perp * screenRadius,
        s2 - perp * screenRadius,
        s2 + perp * screenRadius,
    };

    dc->renderer->FillConvexPolygon(rectCorners, 4, fillC);

    auto outlineC = Color::Unpack(color, 255);
    dc->renderer->SetDrawColor(outlineC);
    dc->renderer->DrawLine(Rect{rectCorners[0].x, rectCorners[0].y, rectCorners[1].x, rectCorners[1].y});
    dc->renderer->DrawLine(Rect{rectCorners[1].x, rectCorners[1].y, rectCorners[2].x, rectCorners[2].y});
    dc->renderer->DrawLine(Rect{rectCorners[2].x, rectCorners[2].y, rectCorners[3].x, rectCorners[3].y});
    dc->renderer->DrawLine(Rect{rectCorners[3].x, rectCorners[3].y, rectCorners[0].x, rectCorners[0].y});
}

void RenderSystem::OnDebugDrawSegment(Vector2D p1, Vector2D p2, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto s1 = dc->viewport->WorldToScreen(p1);
    auto s2 = dc->viewport->WorldToScreen(p2);

    auto c = Color::Unpack(color, 255);
    dc->renderer->SetDrawColor(c);
    dc->renderer->DrawLine(Rect{s1.x, s1.y, s2.x, s2.y});
}

void RenderSystem::OnDebugDrawTransform(Vector2D position, float angle, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto origin = dc->viewport->WorldToScreen(position);
    float ppm = dc->viewport->GetPixelsPerMeter() * dc->viewport->GetMainCamera()->GetZoom();
    float axisLen = 0.5f * ppm;

    float cosA = cosf(angle);
    float sinA = sinf(angle);

    dc->renderer->SetDrawColor(Color(255, 0, 0, 255));
    dc->renderer->DrawLine(Rect{origin.x, origin.y, origin.x + axisLen * cosA, origin.y + axisLen * sinA});

    dc->renderer->SetDrawColor(Color(0, 255, 0, 255));
    dc->renderer->DrawLine(Rect{origin.x, origin.y, origin.x - axisLen * sinA, origin.y + axisLen * cosA});
}

void RenderSystem::OnDebugDrawPoint(Vector2D p, float size, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto screen = dc->viewport->WorldToScreen(p);
    float ppm = dc->viewport->GetPixelsPerMeter() * dc->viewport->GetMainCamera()->GetZoom();
    float screenSize = size * ppm;

    auto c = Color::Unpack(color, 255);
    dc->renderer->SetDrawColor(c);

    for (float dy = -screenSize; dy <= screenSize; dy++)
        for (float dx = -screenSize; dx <= screenSize; dx++)
            if (dx * dx + dy * dy <= screenSize * screenSize)
                dc->renderer->DrawPoint(screen.x + dx, screen.y + dy);
}

} // namespace Aeon
