#include <runtime/ecs/ecs_render_system.h>

#include <imgui.h>

#include <modules/assets/asset.h>
#include <modules/assets/asset_manager.h>
#include <modules/video/image.h>
#include <modules/video/viewport.h>
#include <ncore/modules/video/render_service.h>
#include <ncore/runtime/ecs/ecs_rigidbody.h>
#include <ncore/runtime/ecs/ecs_sprite.h>
#include <ncore/runtime/ecs/ecs_transform.h>
#include <ncore/runtime/ecs_world.h>
#include <ncore/runtime/service_locator.h>

namespace ncore {

void EcsRenderSystem::on_init(EcsWorld &world) {
    renderer = world.get_services().resolve<IRenderService>();
    resources = world.get_services().resolve<AssetManager>();
    physics = world.get_services().resolve<IPhysicsService>();
    viewport = world.get_viewport();

    debug_draw_ctx.renderer = renderer;
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
}

void EcsRenderSystem::on_render(EcsWorld &world, IRenderService &graphics) {
    (void) world;
    (void) graphics;
    physics->update_debug_draw();
    // TODO: iterate entities once ecs_world.cpp is implemented
}

void EcsRenderSystem::on_post_update(EcsWorld &world, double delta) {}

void EcsRenderSystem::on_gui_render(EcsWorld &world) {
    ImGui::Begin("Camera");

    if (viewport) {
        auto &cam = viewport->get_camera();
        ImGui::Text("Position: (%.2f, %.2f)", cam.get_position().x, cam.get_position().y);
        ImGui::Text("Zoom: %.2f", cam.get_zoom());
    }

    ImGui::End();
}

// void EcsRenderSystem::render_sprite(EcsWorld &world, EcsEntity *entityPtr) {
//     auto &transform = world.get<EcsTransform>(*entityPtr);
//     auto &sprite = world.get<EcsSprite>(*entityPtr);
//
//     if (!sprite.res.is_valid()) {
//         sprite.res = resources->load<Image>(std::string(sprite.filepath));
//         sprite.rect.w = static_cast<int>(transform.dimension.x * transform.scale);
//         sprite.rect.h = static_cast<int>(transform.dimension.y * transform.scale);
//     }
//
//     float zoom = viewport->get_camera().get_zoom();
//     float scaledWidth = transform.dimension.x * transform.scale * viewport->get_pixels_per_meter() * zoom;
//     float scaledHeight = transform.dimension.y * transform.scale * viewport->get_pixels_per_meter() * zoom;
//
//     Vec2 screen_pos = viewport->world_to_screen(transform.position);
//
//     sprite.rect.w = static_cast<int>(scaledWidth);
//     sprite.rect.h = static_cast<int>(scaledHeight);
//     sprite.rect.x = static_cast<int>(screen_pos.x - (scaledWidth / 2.0f));
//     sprite.rect.y = static_cast<int>(screen_pos.y - (scaledHeight / 2.0f));
//
//     renderer->draw_texture(resources->get<Image>(sprite.res), sprite.rect, Vec4(), transform.angle, Color());
// }
//
// void EcsRenderSystem::render_temp_circle(EcsWorld &world, EcsEntity *entityPtr) {
//     auto &transform = world.get<EcsTransform>(*entityPtr);
//     auto &circle = world.get<EcsCircleDraw>(*entityPtr);
//
//     float zoom = viewport->get_camera().get_zoom();
//     float scaled_radius = circle.radius * viewport->get_pixels_per_meter() * zoom;
//
//     Vec2 screen_pos = viewport->world_to_screen(transform.position);
//     renderer->draw_circle(screen_pos.x, screen_pos.y, scaled_radius, circle.color, circle.filled, circle.edge);
// }
//
// void EcsRenderSystem::render_phys_debug(EcsWorld &world, EcsEntity *entityPtr) {
//     auto &transform = world.get<EcsTransform>(*entityPtr);
//     auto &body = world.get<EcsRigidbody>(*entityPtr);
//
//     float zoom = viewport->get_camera().get_zoom();
//     Vec2 screen_pos = viewport->world_to_screen(transform.position);
//
//     Vec2 scaled_vel = body.velocity * viewport->get_pixels_per_meter() * zoom;
//
//     renderer->set_draw_color(Color(255, 0, 0, 255));
//     renderer->draw_line(Vec4{screen_pos.x, screen_pos.y, screen_pos.x + scaled_vel.x, screen_pos.y + scaled_vel.y});
// }

void EcsRenderSystem::OnDebugDrawPolygon(const Vec2 *vertices, int vertexCount, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer || vertexCount < 2 || !dc->viewport)
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

void EcsRenderSystem::OnDebugDrawSolidPolygon(const Vec2 *vertices, int vertexCount, float radius, uint32_t color,
                                              void *context) {
    (void) radius;
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer || vertexCount < 3 || !dc->viewport)
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

void EcsRenderSystem::OnDebugDrawCircle(Vec2 center, float radius, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer || !dc->viewport)
        return;

    auto screen_pos = dc->viewport->world_to_screen(center);
    float screen_radius = radius * dc->viewport->get_pixels_per_meter() * dc->viewport->get_camera().get_zoom();

    auto c = Color::unpack(color, 255);
    dc->renderer->draw_circle(screen_pos.x, screen_pos.y, screen_radius, c, false, false);
}

void EcsRenderSystem::OnDebugDrawSolidCircle(Vec2 center, float radius, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer || !dc->viewport)
        return;

    auto screen_pos = dc->viewport->world_to_screen(center);
    float screen_radius = radius * dc->viewport->get_pixels_per_meter() * dc->viewport->get_camera().get_zoom();

    auto fillC = Color::unpack(color, 120);
    dc->renderer->draw_circle(screen_pos.x, screen_pos.y, screen_radius, fillC, true, false);

    auto outlineC = Color::unpack(color, 255);
    dc->renderer->draw_circle(screen_pos.x, screen_pos.y, screen_radius, outlineC, false, true);
}

void EcsRenderSystem::OnDebugDrawSolidCapsule(Vec2 p1, Vec2 p2, float radius, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer || !dc->viewport)
        return;

    auto s1 = dc->viewport->world_to_screen(p1);
    auto s2 = dc->viewport->world_to_screen(p2);
    float ppm = dc->viewport->get_pixels_per_meter() * dc->viewport->get_camera().get_zoom();
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

void EcsRenderSystem::OnDebugDrawSegment(Vec2 p1, Vec2 p2, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto s1 = dc->viewport->world_to_screen(p1);
    auto s2 = dc->viewport->world_to_screen(p2);

    auto c = Color::unpack(color, 255);
    dc->renderer->set_draw_color(c);
    dc->renderer->draw_line(Vec4{s1.x, s1.y, s2.x, s2.y});
}

void EcsRenderSystem::OnDebugDrawTransform(Vec2 position, float angle, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer || !dc->viewport)
        return;

    auto origin = dc->viewport->world_to_screen(position);
    float ppm = dc->viewport->get_pixels_per_meter() * dc->viewport->get_camera().get_zoom();
    float axisLen = 0.5f * ppm;

    float cosA = cosf(angle);
    float sinA = sinf(angle);

    dc->renderer->set_draw_color(Color(255, 0, 0, 255));
    dc->renderer->draw_line(Vec4{origin.x, origin.y, origin.x + axisLen * cosA, origin.y + axisLen * sinA});

    dc->renderer->set_draw_color(Color(0, 255, 0, 255));
    dc->renderer->draw_line(Vec4{origin.x, origin.y, origin.x - axisLen * sinA, origin.y + axisLen * cosA});
}

void EcsRenderSystem::OnDebugDrawPoint(Vec2 p, float size, uint32_t color, void *context) {
    auto *dc = static_cast<PhysDebugDrawContext *>(context);
    if (!dc->renderer)
        return;

    auto screen = dc->viewport->world_to_screen(p);
    float ppm = dc->viewport->get_pixels_per_meter() * dc->viewport->get_camera().get_zoom();
    float screen_size = size * ppm;

    auto c = Color::unpack(color, 255);
    dc->renderer->set_draw_color(c);

    for (float dy = -screen_size; dy <= screen_size; dy++)
        for (float dx = -screen_size; dx <= screen_size; dx++)
            if (dx * dx + dy * dy <= screen_size * screen_size)
                dc->renderer->draw_point(screen.x + dx, screen.y + dy);
}

} // namespace ncore
