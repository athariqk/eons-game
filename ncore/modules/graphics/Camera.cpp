#include "Camera.h"

namespace ncore {

Camera2D::Camera2D() : position(0.0f, 0.0f), zoom(1.0f) {}

Camera2D::Camera2D(const Vec2 &position, float zoom) : position(position), zoom(zoom) {}

void Camera2D::move(const Vec2 &delta) {
    position.x += delta.x;
    position.y += delta.y;
}

void Camera2D::move(float dx, float dy) {
    position.x += dx;
    position.y += dy;
}

} // namespace ncore
