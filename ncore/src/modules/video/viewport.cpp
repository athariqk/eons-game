#include <ncore/modules/video/viewport.h>

namespace ncore {

Viewport::Viewport(float ppm) : pixels_per_meter(ppm) {}

Viewport::Viewport(Vec4 rect) : view_rect(rect) {}

Viewport::~Viewport() = default;

void Viewport::set_rect(Vec4 rect) { view_rect = rect; }

void Viewport::set_position(float x, float y) {
    view_rect.x = x;
    view_rect.y = y;
}

void Viewport::set_size(float width, float height) {
    view_rect.w = width;
    view_rect.h = height;
}

Vec2 Viewport::world_to_screen(const Vec2 &worldPos) const {
    Vec2 screenPos;
    screenPos.x = (worldPos.x - camera.get_position().x) * pixels_per_meter * camera.get_zoom();
    screenPos.y = (worldPos.y - camera.get_position().y) * pixels_per_meter * camera.get_zoom();
    screenPos.x += view_rect.w / 2.0f + view_rect.x;
    screenPos.y += view_rect.h / 2.0f + view_rect.y;
    return screenPos;
}

Vec2 Viewport::screen_to_world(const Vec2 &screenPos) const {
    Vec2 worldPos;
    worldPos.x = ((screenPos.x - view_rect.x - view_rect.w / 2.0f) / camera.get_zoom()) / pixels_per_meter +
                 camera.get_position().x;
    worldPos.y = ((screenPos.y - view_rect.y - view_rect.h / 2.0f) / camera.get_zoom()) / pixels_per_meter +
                 camera.get_position().y;
    return worldPos;
}

bool Viewport::get_is_point_visible(const Vec2 &worldPos) const {
    const auto screenPos = world_to_screen(worldPos);
    return screenPos.x >= view_rect.x && screenPos.x <= view_rect.x + view_rect.w && screenPos.y >= view_rect.y &&
           screenPos.y <= view_rect.y + view_rect.h;
}

bool Viewport::get_is_rect_visible(const Vec2 &worldPos, const Vec2 &size) const {
    Vec2 worldMin = screen_to_world(get_position());
    Vec2 worldMax = screen_to_world(Vec2(view_rect.x + view_rect.w, view_rect.y + view_rect.h));
    return worldPos.x + size.x >= worldMin.x && worldPos.x <= worldMax.x && worldPos.y + size.y >= worldMin.y &&
           worldPos.y <= worldMax.y;
}

} // namespace ncore
