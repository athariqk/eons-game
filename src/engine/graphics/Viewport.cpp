//
// Created by athariqk on 04/03/2025.
//
#include "Viewport.h"

#include <SDLGraphicsContext.h>
#include "Camera.h"

namespace Aeon {

Viewport2D::Viewport2D(Window &p_window) : m_x(0.0f), m_y(0.0f), m_width(0.0f), m_height(0.0f) { Init(p_window); }

Viewport2D::Viewport2D(Window &p_window, float x, float y, float width, float height) :
    m_x(x), m_y(y), m_width(width), m_height(height) {
    Init(p_window);
}

void Viewport2D::Init(Window &p_window) {
    // Create default main camera at origin with zoom 1.0
    m_mainCamera = std::make_unique<Camera2D>();

    // TODO: hardcode SDL as our only graphics context for now
    m_graphicsContext = std::make_unique<SDLGraphicsContext>(p_window.GetRenderer());
    m_window = p_window.GetSDLWindow();
}

Viewport2D::~Viewport2D() = default;

void Viewport2D::SetRect(float x, float y, float width, float height) {
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

void Viewport2D::SetPosition(float x, float y) {
    m_x = x;
    m_y = y;
}

void Viewport2D::SetSize(float width, float height) {
    m_width = width;
    m_height = height;
}

Vector2D Viewport2D::WorldToScreen(const Vector2D &worldPos) const {
    if (!m_mainCamera) {
        return worldPos;
    }

    // Calculate relative position to camera
    Vector2D screenPos;
    screenPos.x = (worldPos.x - m_mainCamera->GetPosition().x) * m_pixelsPerMeter * m_mainCamera->GetZoom();
    screenPos.y = (worldPos.y - m_mainCamera->GetPosition().y) * m_pixelsPerMeter * m_mainCamera->GetZoom();

    // Add pixel offsets to center everything perfectly on screen
    screenPos.x += m_width / 2.0f + m_x;
    screenPos.y += m_height / 2.0f + m_y;

    return screenPos;
}

Vector2D Viewport2D::ScreenToWorld(const Vector2D &screenPos) const {
    if (!m_mainCamera) {
        return screenPos;
    }

    Vector2D worldPos;
    worldPos.x = ((screenPos.x - m_x - m_width / 2.0f) / m_mainCamera->GetZoom()) / m_pixelsPerMeter +
                 m_mainCamera->GetPosition().x;
    worldPos.y = ((screenPos.y - m_y - m_height / 2.0f) / m_mainCamera->GetZoom()) / m_pixelsPerMeter +
                 m_mainCamera->GetPosition().y;

    return worldPos;
}

bool Viewport2D::IsPointVisible(const Vector2D &worldPos) const {
    const auto screenPos = WorldToScreen(worldPos);

    return screenPos.x >= m_x && screenPos.x <= m_x + m_width && screenPos.y >= m_y && screenPos.y <= m_y + m_height;
}

bool Viewport2D::IsRectVisible(const Vector2D &worldPos, const Vector2D &size) const {
    if (!m_mainCamera) {
        return true;
    }

    // Get world bounds visible by this camera
    Vector2D worldMin = ScreenToWorld(GetPosition());
    Vector2D worldMax = ScreenToWorld(Vector2D(m_x + m_width, m_y + m_height));

    // Check if rectangles overlap
    return worldPos.x + size.x >= worldMin.x && worldPos.x <= worldMax.x && worldPos.y + size.y >= worldMin.y &&
           worldPos.y <= worldMax.y;
}

} // namespace Aeon
