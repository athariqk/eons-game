#pragma once

#include "ICamera.h"
#include "Vector2D.h"

//! \brief Default 2D camera implementation
class Camera2D : public ICamera {
public:
    Camera2D();
    Camera2D(const Vector2D &position, float zoom = 1.0f);
    ~Camera2D() override = default;

    // ICamera interface
    Vector2D GetPosition() const override { return m_position; }
    float GetZoom() const override { return m_zoom; }
    void SetPosition(const Vector2D &position) override { m_position = position; }
    void SetZoom(float zoom) override { m_zoom = zoom; }

    // Camera controls
    void Move(const Vector2D &delta);
    void Move(float dx, float dy);

private:
    Vector2D m_position{0.0f, 0.0f};
    float m_zoom = 1.0f;
};
