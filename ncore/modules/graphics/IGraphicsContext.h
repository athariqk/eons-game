#pragma once

#include <cstdint>

#include <modules/utils/Color.h>
#include <modules/utils/Structures.h>

namespace ncore {

struct Image;

class IGraphicsContext {
public:
    virtual ~IGraphicsContext() = default;

    // Basic rendering
    virtual void clear() = 0;
    virtual void present() = 0;
    virtual void set_draw_color(const Color &color) = 0;

    // Shape drawing
    virtual void draw_line(const Vec4 &rect) = 0;
    virtual void draw_rect(const Vec4 &rect) = 0;
    virtual void fill_rect(const Vec4 &rect) = 0;
    virtual void draw_point(float x, float y) = 0;

    // More advanced rendering
    virtual void draw_texture(const Image *tex, const Vec4 &destRect, const Vec4 &srcRect, float angle,
                              const Color &color) = 0;

    void draw_circle(float x, float y, float radius, Color color, bool filled, bool edge);
    void draw_convex_polygon_filled(const Vec2 *vertices, int count, const Color &color);

    // Get implementation handle (for internal use only by engine systems)
    virtual void *get_native_handle() const = 0;
};

} // namespace ncore
