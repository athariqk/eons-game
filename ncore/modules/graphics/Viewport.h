#pragma once

#include <memory>

#include <SDL3/SDL_render.h>

#include <modules/graphics/ICamera.h>
#include <modules/graphics/IGraphicsContext.h>
#include <modules/graphics/Window.h>

namespace ncore {

//! \brief Defines the screen rectangle where rendering occurs
//! Also manages the main camera for view transformation
class Viewport2D {
public:
    Viewport2D(Window &p_window, float ppm);
    Viewport2D(Window &p_window, Vec4 rect);
    ~Viewport2D();

    // Set/Get viewport rectangle
    void set_rect(Vec4 rect);
    void set_position(float x, float y);
    void set_size(float width, float height);

    Vec4 get_rect() const { return view_rect; }
    Vec2 get_position() const { return Vec2(view_rect.x, view_rect.y); }
    Vec2 get_size() const { return Vec2(view_rect.w, view_rect.h); }
    Vec2 get_center() const { return Vec2(view_rect.x + view_rect.w * 0.5f, view_rect.y + view_rect.h * 0.5f); }

    ICamera *get_main_camera() { return main_cam.get(); }
    const ICamera *get_main_camera() const { return main_cam.get(); }
    void set_main_camera(std::unique_ptr<ICamera> camera) { main_cam = std::move(camera); }

    Vec2 world_to_screen(const Vec2 &worldPos) const;
    Vec2 screen_to_world(const Vec2 &screenPos) const;

    float get_pixels_per_meter() const { return pixels_per_meter; }
    void set_pixels_per_meter(float ppm) { pixels_per_meter = ppm; }

    bool get_is_point_visible(const Vec2 &worldPos) const;
    bool get_is_rect_visible(const Vec2 &worldPos, const Vec2 &size) const;

    // System access through interfaces
    IGraphicsContext *get_graphics_context() const { return graphics_ctx.get(); }
    SDL_Window *get_native_window() const { return m_window; } // HACK: For ImGui which needs SDL_Window

private:
    void init(Window &p_window);

private:
    Vec4 view_rect;
    float pixels_per_meter = 32.0f; // default scale factor for world-to-screen conversion
    std::unique_ptr<IGraphicsContext> graphics_ctx;
    std::unique_ptr<ICamera> main_cam;
    SDL_Window *m_window = nullptr; // HACK: Needed for ImGui initialization
};

} // namespace ncore
