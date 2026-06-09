#pragma once

#include <cstdint>

#include <Color.h>
#include <Vector2D.h>

namespace Aeon {

struct Rect {
    float x, y, w, h;
};

class IGraphicsContext {
public:
    virtual ~IGraphicsContext() = default;

    // Basic rendering
    virtual void Clear() = 0;
    virtual void Present() = 0;
    virtual void SetDrawColor(const Color &color) = 0;

    // Shape drawing
    virtual void DrawLine(const Rect &rect) = 0;
    virtual void DrawRect(const Rect &rect) = 0;
    virtual void FillRect(const Rect &rect) = 0;
    virtual void DrawPoint(float x, float y) = 0;

    void DrawCircle(float x, float y, float radius, Color color, bool filled, bool edge);
    void FillConvexPolygon(const Vector2D *vertices, int count, const Color &color);

    // Get implementation handle (for internal use only by engine systems)
    virtual void *GetNativeHandle() const = 0;
};

} // namespace Aeon
