#include <ncore/modules/video/camera.h>

namespace ncore {

Camera::Camera() : position(0.0f, 0.0f), zoom(1.0f) {}

Camera::Camera(const Vec2& position, float zoom) : position(position), zoom(zoom) {}

void Camera::move(const Vec2& delta)
{
    position.x += delta.x;
    position.y += delta.y;
}

void Camera::move(float dx, float dy)
{
    position.x += dx;
    position.y += dy;
}

} // namespace ncore
