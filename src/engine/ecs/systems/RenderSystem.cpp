#include "RenderSystem.h"

#include <Logger.h>
#include <RigidBodyComponent.h>
#include <SpriteComponent.h>
#include <TextureManager.h>
#include <TransformComponent.h>
#include <World.h>
#include <cmath>
#include <imgui.h>

namespace ncore {

bool RenderSystem::on_init(World &world) {
    viewport = world.get_main_loop().get_services().try_get<Viewport2D>();
    if (!viewport) {
        LOG_ERROR(log::GRAPHICS, "Viewport2D service not found!");
        return false;
    }

    graphics_ctx = viewport->get_graphics_context();
    if (!graphics_ctx) {
        LOG_ERROR(log::GRAPHICS, "Graphics context not found in Viewport2D!");
        return false;
    }

    texture_mgr = world.get_main_loop().get_services().try_get<TextureManager>();
    if (!texture_mgr) {
        LOG_ERROR(log::GRAPHICS, "TextureManager not found!");
        return false;
    }

    physics = world.get_main_loop().get_services().try_get<Physics2D>();
    if (!physics) {
        LOG_ERROR(log::GRAPHICS, "Physics2D service not found!");
        return false;
    }

    debug_draw_ctx.renderer = graphics_ctx;
    debug_draw_ctx.viewport = viewport;

    auto &fnc = physics->get_debug_draw_fnc();
    fnc.context = &debug_draw_ctx;
    fnc.draw_polygon = OnDebugDrawPolygon;
    fnc.draw_solid_polygon = OnDebugDrawSolidPolygon;
    fnc.draw_circle = OnDebugDrawCircle;
    fnc.draw_solid_circle = OnDebugDrawSolidCircle;
    fnc.draw_solid_capsule = OnDebugDrawSolidCapsule;
    fnc.draw_segment = OnDebugDrawSegment;
    fnc.draw_transform = OnDebugDrawTransform;
    fnc.draw_point = OnDebugDrawPoint;

    LOG_INFO(log::GRAPHICS, "RenderSystem initialized");

    return true;
}

void RenderSystem::on_render(World &world, IGraphicsContext &graphics) {
    physics->update_debug_draw();

    for (const auto &entityPtr: world.get_entities()) {
        if (!entityPtr->is_enabled())
            continue;

        if (entityPtr->has_component<SpriteComponent>()) {
            render_sprite(entityPtr.get());
        }

        if (entityPtr->has_component<TempCircleComponent>()) {
            render_temp_circle(entityPtr.get());
        }

        if (physics->is_debug_draw && entityPtr->has_component<RigidBodyComponent>()) {
            render_phys_debug(entityPtr.get());
        }
    }
}

void RenderSystem::on_post_update(World &world, double delta) {}

void RenderSystem::on_gui_render(World &world) {
    ImGui::Begin("Rendering");

    if (auto cam = viewport->get_main_camera()) {
        const auto &camPos = cam->get_position();
        ImGui::Text("Camera position: (x: %f, y: %f)", camPos.x, camPos.y);

        float zoom = cam->get_zoom();
        ImGui::Text("Camera zoom: %f", zoom);
    }

    ImGui::End();
}

void RenderSystem::render_sprite(Entity *entityPtr) {
    auto renderer = static_cast<SDL_Renderer *>(graphics_ctx->get_native_handle());
    if (!renderer) {
        LOG_ERROR(log::GRAPHICS, "Graphics renderer not initialized!");
        return;
    }

    if (!entityPtr->has_component<TransformComponent>()) {
        LOG_ERROR(log::GRAPHICS, "Entity {} has no TransformComponent to render with!", entityPtr->get_id());
        return;
    }

    auto &transform = entityPtr->get_component<TransformComponent>();
    auto &sprite = entityPtr->get_component<SpriteComponent>();

    if (!sprite.texture_ptr) {
        sprite.texture_ptr = texture_mgr->GetTexture(sprite.texture_path);
        if (!sprite.texture_ptr) {
            return;
        }
        sprite.rect.w = transform.dimension.x * transform.scale;
        sprite.rect.h = transform.dimension.y * transform.scale;
    }

    float zoom = viewport->get_main_camera()->get_zoom();

    float scaledWidth = transform.dimension.x * transform.scale * viewport->get_pixels_per_meter() * zoom;
    float scaledHeight = transform.dimension.y * transform.scale * viewport->get_pixels_per_meter() * zoom;
    float scaledAngle = transform.angle * transform.scale * viewport->get_pixels_per_meter() * zoom;

    Vec2D screenPos = viewport->world_to_screen(transform.position);

    sprite.rect.w = static_cast<int>(scaledWidth);
    sprite.rect.h = static_cast<int>(scaledHeight);
    sprite.rect.x = static_cast<int>(screenPos.x - (scaledWidth / 2.0f));
    sprite.rect.y = static_cast<int>(screenPos.y - (scaledHeight / 2.0f));

    auto sdlTexture = static_cast<SDL_Texture *>(sprite.texture_ptr);
    auto sdlRect = SDL_FRect{sprite.rect.x, sprite.rect.y, sprite.rect.w, sprite.rect.h};
    TextureManager::Draw(renderer, sdlTexture, nullptr, &sdlRect, scaledAngle);
}

void RenderSystem::render_temp_circle(Entity *entityPtr) {
    if (!entityPtr->has_component<TransformComponent>()) {
        LOG_ERROR(log::GRAPHICS, "Entity {} has no TransformComponent to render with!", entityPtr->get_id());
        return;
    }

    auto &transform = entityPtr->get_component<TransformComponent>();
    auto &circle = entityPtr->get_component<TempCircleComponent>();

    float zoom = viewport->get_main_camera()->get_zoom();
    float scaledRadius = circle.radius * viewport->get_pixels_per_meter() * zoom;

    Vec2D screenPos = viewport->world_to_screen(transform.position);
    graphics_ctx->draw_circle(screenPos.x, screenPos.y, scaledRadius, circle.color, circle.filled, circle.edge);
}

/**
 * TODO: remove this and move ALL physics debug drawing in the Physics class
 */
void RenderSystem::render_phys_debug(Entity *entityPtr) {
    if (!entityPtr->has_component<TransformComponent>()) {
        LOG_ERROR(log::GRAPHICS, "Entity {} has no TransformComponent to render with!", entityPtr->get_id());
        return;
    }

    auto &transform = entityPtr->get_component<TransformComponent>();
    auto &body = entityPtr->get_component<RigidBodyComponent>();
    auto [vx, vy] = body.velocity;

    float zoom = viewport->get_main_camera()->get_zoom();
    Vec2D screenPos = viewport->world_to_screen(transform.position);

    float scaledWidth = transform.dimension.x * viewport->get_pixels_per_meter() * zoom;
    Vec2D scaledVel = body.velocity * viewport->get_pixels_per_meter() * zoom;

    graphics_ctx->set_draw_color(Color(255, 0, 0, 255));
    graphics_ctx->draw_line(Rect{screenPos.x, screenPos.y, screenPos.x + scaledVel.x, screenPos.y + scaledVel.y});
    /*PrimitiveShape::DrawRectangle(renderer, screenPos.x - (scaledWidth * 0.5f), screenPos.y - (scaledWidth * 0.5f),
                                  scaledWidth);*/
}

// ---- DebugDrawFnc callbacks ----

void RenderSystem::OnDebugDrawPolygon(const Vec2D *vertices, int vertexCount, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer || vertexCount < 2)
        return;

    auto c = Color::unpack(color, 255);
    dc->renderer->set_draw_color(c);

    Vec2D screen[8];
    for (int i = 0; i < vertexCount; i++)
        screen[i] = dc->viewport->world_to_screen(vertices[i]);

    for (int i = 0; i < vertexCount - 1; i++)
        dc->renderer->draw_line(Rect{screen[i].x, screen[i].y, screen[i + 1].x, screen[i + 1].y});

    dc->renderer->draw_line(Rect{screen[vertexCount - 1].x, screen[vertexCount - 1].y, screen[0].x, screen[0].y});
}

void RenderSystem::OnDebugDrawSolidPolygon(const Vec2D *vertices, int vertexCount, float radius, uint32_t color,
                                           void *context) {
    (void) radius;
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer || vertexCount < 3)
        return;

    Vec2D screen[8];
    for (int i = 0; i < vertexCount; i++)
        screen[i] = dc->viewport->world_to_screen(vertices[i]);

    auto fillColor = Color::unpack(color, 100);
    dc->renderer->draw_convex_polygon_filled(screen, vertexCount, fillColor);

    auto outlineColor = Color::unpack(color, 255);
    dc->renderer->set_draw_color(outlineColor);

    for (int i = 0; i < vertexCount - 1; i++)
        dc->renderer->draw_line(Rect{screen[i].x, screen[i].y, screen[i + 1].x, screen[i + 1].y});

    dc->renderer->draw_line(Rect{screen[vertexCount - 1].x, screen[vertexCount - 1].y, screen[0].x, screen[0].y});
}

void RenderSystem::OnDebugDrawCircle(Vec2D center, float radius, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto screenPos = dc->viewport->world_to_screen(center);
    float screenRadius = radius * dc->viewport->get_pixels_per_meter() * dc->viewport->get_main_camera()->get_zoom();

    auto c = Color::unpack(color, 255);
    dc->renderer->draw_circle(screenPos.x, screenPos.y, screenRadius, c, false, false);
}

void RenderSystem::OnDebugDrawSolidCircle(Vec2D center, float radius, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto screenPos = dc->viewport->world_to_screen(center);
    float screenRadius = radius * dc->viewport->get_pixels_per_meter() * dc->viewport->get_main_camera()->get_zoom();

    auto fillC = Color::unpack(color, 120);
    dc->renderer->draw_circle(screenPos.x, screenPos.y, screenRadius, fillC, true, false);

    auto outlineC = Color::unpack(color, 255);
    dc->renderer->draw_circle(screenPos.x, screenPos.y, screenRadius, outlineC, false, true);
}

void RenderSystem::OnDebugDrawSolidCapsule(Vec2D p1, Vec2D p2, float radius, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto s1 = dc->viewport->world_to_screen(p1);
    auto s2 = dc->viewport->world_to_screen(p2);
    float ppm = dc->viewport->get_pixels_per_meter() * dc->viewport->get_main_camera()->get_zoom();
    float screenRadius = radius * ppm;

    auto fillC = Color::unpack(color, 120);
    dc->renderer->draw_circle(s1.x, s1.y, screenRadius, fillC, true, false);
    dc->renderer->draw_circle(s2.x, s2.y, screenRadius, fillC, true, false);

    Vec2D dir = s2 - s1;
    float len = dir.length();
    if (len < 0.001f)
        return;
    dir = dir * (1.0f / len);

    Vec2D perp(-dir.y, dir.x);
    Vec2D rectCorners[4] = {
        s1 + perp * screenRadius,
        s1 - perp * screenRadius,
        s2 - perp * screenRadius,
        s2 + perp * screenRadius,
    };

    dc->renderer->draw_convex_polygon_filled(rectCorners, 4, fillC);

    auto outlineC = Color::unpack(color, 255);
    dc->renderer->set_draw_color(outlineC);
    dc->renderer->draw_line(Rect{rectCorners[0].x, rectCorners[0].y, rectCorners[1].x, rectCorners[1].y});
    dc->renderer->draw_line(Rect{rectCorners[1].x, rectCorners[1].y, rectCorners[2].x, rectCorners[2].y});
    dc->renderer->draw_line(Rect{rectCorners[2].x, rectCorners[2].y, rectCorners[3].x, rectCorners[3].y});
    dc->renderer->draw_line(Rect{rectCorners[3].x, rectCorners[3].y, rectCorners[0].x, rectCorners[0].y});
}

void RenderSystem::OnDebugDrawSegment(Vec2D p1, Vec2D p2, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto s1 = dc->viewport->world_to_screen(p1);
    auto s2 = dc->viewport->world_to_screen(p2);

    auto c = Color::unpack(color, 255);
    dc->renderer->set_draw_color(c);
    dc->renderer->draw_line(Rect{s1.x, s1.y, s2.x, s2.y});
}

void RenderSystem::OnDebugDrawTransform(Vec2D position, float angle, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto origin = dc->viewport->world_to_screen(position);
    float ppm = dc->viewport->get_pixels_per_meter() * dc->viewport->get_main_camera()->get_zoom();
    float axisLen = 0.5f * ppm;

    float cosA = cosf(angle);
    float sinA = sinf(angle);

    dc->renderer->set_draw_color(Color(255, 0, 0, 255));
    dc->renderer->draw_line(Rect{origin.x, origin.y, origin.x + axisLen * cosA, origin.y + axisLen * sinA});

    dc->renderer->set_draw_color(Color(0, 255, 0, 255));
    dc->renderer->draw_line(Rect{origin.x, origin.y, origin.x - axisLen * sinA, origin.y + axisLen * cosA});
}

void RenderSystem::OnDebugDrawPoint(Vec2D p, float size, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto screen = dc->viewport->world_to_screen(p);
    float ppm = dc->viewport->get_pixels_per_meter() * dc->viewport->get_main_camera()->get_zoom();
    float screenSize = size * ppm;

    auto c = Color::unpack(color, 255);
    dc->renderer->set_draw_color(c);

    for (float dy = -screenSize; dy <= screenSize; dy++)
        for (float dx = -screenSize; dx <= screenSize; dx++)
            if (dx * dx + dy * dy <= screenSize * screenSize)
                dc->renderer->draw_point(screen.x + dx, screen.y + dy);
}

} // namespace ncore

