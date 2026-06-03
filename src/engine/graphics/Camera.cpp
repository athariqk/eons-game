#include "Camera.h"

namespace Aeon {

Camera2D::Camera2D() : m_position(0.0f, 0.0f), m_zoom(1.0f) {}

Camera2D::Camera2D(const Vector2D &position, float zoom) : m_position(position), m_zoom(zoom) {}

void Camera2D::Move(const Vector2D &delta) {
    m_position.x += delta.x;
    m_position.y += delta.y;
}

void Camera2D::Move(float dx, float dy) {
    m_position.x += dx;
    m_position.y += dy;
}

} // namespace Aeon
