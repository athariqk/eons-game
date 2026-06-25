#pragma once

#include <ncore/kernel/structures.h>

#include "camera.h"

namespace ncore {

//! \brief Defines the screen rectangle where rendering occurs
class Viewport {
public:
    Viewport(float ppm);
    Viewport(Vec4 rect);
    ~Viewport();

    // Set/Get viewport rectangle
    void set_rect(Vec4 rect);
    void set_position(float x, float y);
    void set_size(float width, float height);

    Vec4 get_rect() const
    {
        return view_rect;
    }
    Vec2 get_position() const
    {
        return Vec2(view_rect.x, view_rect.y);
    }
    Vec2 get_size() const
    {
        return Vec2(view_rect.w, view_rect.h);
    }
    Vec2 get_center() const
    {
        return Vec2(view_rect.x + view_rect.w * 0.5f, view_rect.y + view_rect.h * 0.5f);
    }

    Vec2 world_to_screen(const Vec2& worldPos) const;
    Vec2 screen_to_world(const Vec2& screenPos) const;

    float get_pixels_per_meter() const
    {
        return pixels_per_meter;
    }
    void set_pixels_per_meter(float ppm)
    {
        pixels_per_meter = ppm;
    }

    Camera& get_camera()
    {
        return camera;
    }
    const Camera& get_camera() const
    {
        return camera;
    }
    void set_camera_position(const Vec2& pos)
    {
        camera.set_position(pos);
    }
    void set_camera_zoom(float z)
    {
        camera.set_zoom(z);
    }

    bool get_is_point_visible(const Vec2& worldPos) const;
    bool get_is_rect_visible(const Vec2& worldPos, const Vec2& size) const;

private:
    Vec4 view_rect;
    float pixels_per_meter = 32.0f;
    Camera camera;
};

} // namespace ncore
