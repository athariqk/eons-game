#pragma once

#include "ICamera.h"
#include "Vec2D.h"

namespace ncore {

//! \brief Default 2D camera implementation
class Camera2D : public ICamera {
public:
    Camera2D();
    Camera2D(const Vec2D &position, float zoom = 1.0f);
    ~Camera2D() override = default;

    // ICamera interface
    Vec2D get_position() const override { return position; }
    void set_position(const Vec2D &p_position) override { position = p_position; }

	float get_zoom() const override { return zoom; }
    void set_zoom(float p_zoom) override { zoom = p_zoom; }

    // Camera controls
    void move(const Vec2D &delta);
    void move(float dx, float dy);

private:
    Vec2D position;
    float zoom = 1.0f;
};

} // namespace ncore
