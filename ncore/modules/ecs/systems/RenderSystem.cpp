#include "RenderSystem.h"

#include <cmath>
#include <imgui.h>

#include <modules/MainLoop.h>
#include <modules/World.h>
#include <modules/ecs/components/RigidBodyComponent.h>
#include <modules/ecs/components/SpriteComponent.h>
#include <modules/ecs/components/TransformComponent.h>
#include <modules/resources/Resource.h>
#include <modules/resources/ResourceManager.h>

namespace ncore {

bool RenderSystem::on_init(World &world) {
    viewport = world.get_main_loop().get_services().try_get<Viewport2D>();
    NC_ASSERT_RETVAL(viewport != nullptr, false, "Viewport2D service not found!");

    graphics_ctx = viewport->get_graphics_context();
    NC_ASSERT_RETVAL(graphics_ctx != nullptr, false, "GraphicsContext not found in Viewport!");

    res_mgr = world.get_main_loop().get_services().try_get<ResourceManager>();
    NC_ASSERT_RETVAL(res_mgr != nullptr, false, "ResourceManager service not found!");

    physics = world.get_main_loop().get_services().try_get<Physics2D>();
    NC_ASSERT_RETVAL(physics != nullptr, false, "Physics2D service not found!");

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

    return true;
}

void RenderSystem::on_render(World &world, IGraphicsContext &graphics) {
    physics->update_debug_draw();

    for (const auto &entityPtr: world.get_entities()) {
        if (!entityPtr->get_is_enabled())
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
    NC_ASSERT_RET(entityPtr->has_component<SpriteComponent>(),
                  std::format("Entity {} has no SpriteComponent to render!", entityPtr->get_id()).c_str());

    auto &transform = entityPtr->get_component<TransformComponent>();
    auto &sprite = entityPtr->get_component<SpriteComponent>();

    if (!sprite.res.is_valid()) {
        sprite.res = res_mgr->get<Image>(sprite.filepath);
        sprite.rect.w = transform.dimension.x * transform.scale;
        sprite.rect.h = transform.dimension.y * transform.scale;
    }

    float zoom = viewport->get_main_camera()->get_zoom();

    float scaledWidth = transform.dimension.x * transform.scale * viewport->get_pixels_per_meter() * zoom;
    float scaledHeight = transform.dimension.y * transform.scale * viewport->get_pixels_per_meter() * zoom;
    float scaledAngle = transform.angle * transform.scale * viewport->get_pixels_per_meter() * zoom;

    Vec2 screen_pos = viewport->world_to_screen(transform.position);

    sprite.rect.w = static_cast<int>(scaledWidth);
    sprite.rect.h = static_cast<int>(scaledHeight);
    sprite.rect.x = static_cast<int>(screen_pos.x - (scaledWidth / 2.0f));
    sprite.rect.y = static_cast<int>(screen_pos.y - (scaledHeight / 2.0f));

    graphics_ctx->draw_texture(res_mgr->access<Image>(sprite.res), sprite.rect, Vec4(), scaledAngle, Color());
}

void RenderSystem::render_temp_circle(Entity *entityPtr) {
    NC_ASSERT_RET(entityPtr->has_component<TransformComponent>(),
                  std::format("Entity {} has no TransformComponent to render!", entityPtr->get_id()).c_str());

    auto &transform = entityPtr->get_component<TransformComponent>();
    auto &circle = entityPtr->get_component<TempCircleComponent>();

    float zoom = viewport->get_main_camera()->get_zoom();
    float scaled_radius = circle.radius * viewport->get_pixels_per_meter() * zoom;

    Vec2 screen_pos = viewport->world_to_screen(transform.position);
    graphics_ctx->draw_circle(screen_pos.x, screen_pos.y, scaled_radius, circle.color, circle.filled, circle.edge);
}

/**
 * TODO: remove this and move ALL physics debug drawing in the Physics class
 */
void RenderSystem::render_phys_debug(Entity *entityPtr) {
    NC_ASSERT_RET(entityPtr->has_component<TransformComponent>(),
                  std::format("Entity {} has no TransformComponent to render!", entityPtr->get_id()).c_str());

    auto &transform = entityPtr->get_component<TransformComponent>();
    auto &body = entityPtr->get_component<RigidBodyComponent>();
    auto [vx, vy] = body.velocity;

    float zoom = viewport->get_main_camera()->get_zoom();
    Vec2 screen_pos = viewport->world_to_screen(transform.position);

    float scaled_width = transform.dimension.x * viewport->get_pixels_per_meter() * zoom;
    Vec2 scaled_vel = body.velocity * viewport->get_pixels_per_meter() * zoom;

    graphics_ctx->set_draw_color(Color(255, 0, 0, 255));
    graphics_ctx->draw_line(Vec4{screen_pos.x, screen_pos.y, screen_pos.x + scaled_vel.x, screen_pos.y + scaled_vel.y});
    /*PrimitiveShape::DrawRectangle(renderer, screenPos.x - (scaledWidth * 0.5f), screenPos.y - (scaledWidth * 0.5f),
                                  scaledWidth);*/
}

// ---- DebugDrawFnc callbacks ----

void RenderSystem::OnDebugDrawPolygon(const Vec2 *vertices, int vertexCount, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer || vertexCount < 2)
        return;

    auto c = Color::unpack(color, 255);
    dc->renderer->set_draw_color(c);

    Vec2 screen[8];
    for (int i = 0; i < vertexCount; i++)
        screen[i] = dc->viewport->world_to_screen(vertices[i]);

    for (int i = 0; i < vertexCount - 1; i++)
        dc->renderer->draw_line(Vec4{screen[i].x, screen[i].y, screen[i + 1].x, screen[i + 1].y});

    dc->renderer->draw_line(Vec4{screen[vertexCount - 1].x, screen[vertexCount - 1].y, screen[0].x, screen[0].y});
}

void RenderSystem::OnDebugDrawSolidPolygon(const Vec2 *vertices, int vertexCount, float radius, uint32_t color,
                                           void *context) {
    (void) radius;
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer || vertexCount < 3)
        return;

    Vec2 screen[8];
    for (int i = 0; i < vertexCount; i++)
        screen[i] = dc->viewport->world_to_screen(vertices[i]);

    auto fill_color = Color::unpack(color, 100);
    dc->renderer->draw_convex_polygon_filled(screen, vertexCount, fill_color);

    auto outline_color = Color::unpack(color, 255);
    dc->renderer->set_draw_color(outline_color);

    for (int i = 0; i < vertexCount - 1; i++)
        dc->renderer->draw_line(Vec4{screen[i].x, screen[i].y, screen[i + 1].x, screen[i + 1].y});

    dc->renderer->draw_line(Vec4{screen[vertexCount - 1].x, screen[vertexCount - 1].y, screen[0].x, screen[0].y});
}

void RenderSystem::OnDebugDrawCircle(Vec2 center, float radius, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto screen_pos = dc->viewport->world_to_screen(center);
    float screen_radius = radius * dc->viewport->get_pixels_per_meter() * dc->viewport->get_main_camera()->get_zoom();

    auto c = Color::unpack(color, 255);
    dc->renderer->draw_circle(screen_pos.x, screen_pos.y, screen_radius, c, false, false);
}

void RenderSystem::OnDebugDrawSolidCircle(Vec2 center, float radius, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto screen_pos = dc->viewport->world_to_screen(center);
    float screen_radius = radius * dc->viewport->get_pixels_per_meter() * dc->viewport->get_main_camera()->get_zoom();

    auto fillC = Color::unpack(color, 120);
    dc->renderer->draw_circle(screen_pos.x, screen_pos.y, screen_radius, fillC, true, false);

    auto outlineC = Color::unpack(color, 255);
    dc->renderer->draw_circle(screen_pos.x, screen_pos.y, screen_radius, outlineC, false, true);
}

void RenderSystem::OnDebugDrawSolidCapsule(Vec2 p1, Vec2 p2, float radius, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto s1 = dc->viewport->world_to_screen(p1);
    auto s2 = dc->viewport->world_to_screen(p2);
    float ppm = dc->viewport->get_pixels_per_meter() * dc->viewport->get_main_camera()->get_zoom();
    float screenRadius = radius * ppm;

    auto fill_col = Color::unpack(color, 120);
    dc->renderer->draw_circle(s1.x, s1.y, screenRadius, fill_col, true, false);
    dc->renderer->draw_circle(s2.x, s2.y, screenRadius, fill_col, true, false);

    Vec2 dir = s2 - s1;
    float len = dir.length();
    if (len < 0.001f)
        return;
    dir = dir * (1.0f / len);

    Vec2 perp(-dir.y, dir.x);
    Vec2 corners[4] = {
        s1 + perp * screenRadius,
        s1 - perp * screenRadius,
        s2 - perp * screenRadius,
        s2 + perp * screenRadius,
    };

    dc->renderer->draw_convex_polygon_filled(corners, 4, fill_col);

    auto outlineC = Color::unpack(color, 255);
    dc->renderer->set_draw_color(outlineC);
    dc->renderer->draw_line(Vec4{corners[0].x, corners[0].y, corners[1].x, corners[1].y});
    dc->renderer->draw_line(Vec4{corners[1].x, corners[1].y, corners[2].x, corners[2].y});
    dc->renderer->draw_line(Vec4{corners[2].x, corners[2].y, corners[3].x, corners[3].y});
    dc->renderer->draw_line(Vec4{corners[3].x, corners[3].y, corners[0].x, corners[0].y});
}

void RenderSystem::OnDebugDrawSegment(Vec2 p1, Vec2 p2, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto s1 = dc->viewport->world_to_screen(p1);
    auto s2 = dc->viewport->world_to_screen(p2);

    auto c = Color::unpack(color, 255);
    dc->renderer->set_draw_color(c);
    dc->renderer->draw_line(Vec4{s1.x, s1.y, s2.x, s2.y});
}

void RenderSystem::OnDebugDrawTransform(Vec2 position, float angle, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto origin = dc->viewport->world_to_screen(position);
    float ppm = dc->viewport->get_pixels_per_meter() * dc->viewport->get_main_camera()->get_zoom();
    float axisLen = 0.5f * ppm;

    float cosA = cosf(angle);
    float sinA = sinf(angle);

    dc->renderer->set_draw_color(Color(255, 0, 0, 255));
    dc->renderer->draw_line(Vec4{origin.x, origin.y, origin.x + axisLen * cosA, origin.y + axisLen * sinA});

    dc->renderer->set_draw_color(Color(0, 255, 0, 255));
    dc->renderer->draw_line(Vec4{origin.x, origin.y, origin.x - axisLen * sinA, origin.y + axisLen * cosA});
}

void RenderSystem::OnDebugDrawPoint(Vec2 p, float size, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto screen = dc->viewport->world_to_screen(p);
    float ppm = dc->viewport->get_pixels_per_meter() * dc->viewport->get_main_camera()->get_zoom();
    float screen_size = size * ppm;

    auto c = Color::unpack(color, 255);
    dc->renderer->set_draw_color(c);

    for (float dy = -screen_size; dy <= screen_size; dy++)
        for (float dx = -screen_size; dx <= screen_size; dx++)
            if (dx * dx + dy * dy <= screen_size * screen_size)
                dc->renderer->draw_point(screen.x + dx, screen.y + dy);
}

} // namespace ncore
