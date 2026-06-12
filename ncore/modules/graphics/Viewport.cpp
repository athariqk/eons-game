//
// Created by athariqk on 04/03/2025.
//
#include "Viewport.h"

#include <platform/sdl3/graphics/SDLGraphicsContext.h>
#include "modules/graphics/Camera.h"

namespace ncore {

Viewport2D::Viewport2D(Window &p_window, float ppm) : pixels_per_meter(ppm) { init(p_window); }

Viewport2D::Viewport2D(Window &p_window, Vec4 rect) : view_rect(rect) { init(p_window); }

void Viewport2D::init(Window &p_window) {
    // Create default main camera at origin with zoom 1.0
    main_cam = std::make_unique<Camera2D>();

    // TODO: hardcode SDL as our only graphics context for now
    graphics_ctx = std::make_unique<SDLGraphicsContext>(p_window.get_renderer());
    m_window = p_window.get_native_handle();
}

Viewport2D::~Viewport2D() = default;

void Viewport2D::set_rect(Vec4 rect) { view_rect = rect; }

void Viewport2D::set_position(float x, float y) {
    view_rect.x = x;
    view_rect.y = y;
}

void Viewport2D::set_size(float width, float height) {
    view_rect.w = width;
    view_rect.h = height;
}

Vec2 Viewport2D::world_to_screen(const Vec2 &worldPos) const {
    if (!main_cam) {
        return worldPos;
    }

    // Calculate relative position to camera
    Vec2 screenPos;
    screenPos.x = (worldPos.x - main_cam->get_position().x) * pixels_per_meter * main_cam->get_zoom();
    screenPos.y = (worldPos.y - main_cam->get_position().y) * pixels_per_meter * main_cam->get_zoom();

    // Add pixel offsets to center everything perfectly on screen
    screenPos.x += view_rect.w / 2.0f + view_rect.x;
    screenPos.y += view_rect.h / 2.0f + view_rect.y;

    return screenPos;
}

Vec2 Viewport2D::screen_to_world(const Vec2 &screenPos) const {
    if (!main_cam) {
        return screenPos;
    }

    Vec2 worldPos;
    worldPos.x = ((screenPos.x - view_rect.x - view_rect.w / 2.0f) / main_cam->get_zoom()) / pixels_per_meter +
                 main_cam->get_position().x;
    worldPos.y = ((screenPos.y - view_rect.y - view_rect.h / 2.0f) / main_cam->get_zoom()) / pixels_per_meter +
                 main_cam->get_position().y;

    return worldPos;
}

bool Viewport2D::get_is_point_visible(const Vec2 &worldPos) const {
    const auto screenPos = world_to_screen(worldPos);

    return screenPos.x >= view_rect.x && screenPos.x <= view_rect.x + view_rect.w && screenPos.y >= view_rect.y &&
           screenPos.y <= view_rect.y + view_rect.h;
}

bool Viewport2D::get_is_rect_visible(const Vec2 &worldPos, const Vec2 &size) const {
    if (!main_cam) {
        return true;
    }

    // Get world bounds visible by this camera
    Vec2 worldMin = screen_to_world(get_position());
    Vec2 worldMax = screen_to_world(Vec2(view_rect.x + view_rect.w, view_rect.y + view_rect.h));

    // Check if rectangles overlap
    return worldPos.x + size.x >= worldMin.x && worldPos.x <= worldMax.x && worldPos.y + size.y >= worldMin.y &&
           worldPos.y <= worldMax.y;
}

} // namespace ncore
