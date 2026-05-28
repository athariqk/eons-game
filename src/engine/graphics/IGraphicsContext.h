#pragma once

#include <cstdint>

struct Color {
    uint8_t r, g, b, a;
};

struct Rect {
    float x, y, w, h;
};

class IGraphicsContext {
public:
    virtual ~IGraphicsContext() = default;

    // Basic rendering
    virtual void Clear() = 0;
    virtual void Present() = 0;
    virtual void SetDrawColor(const Color& color) = 0;

    // Shape drawing
    virtual void DrawRect(const Rect& rect) = 0;
    virtual void FillRect(const Rect& rect) = 0;
    virtual void DrawPoint(float x, float y) = 0;

    // Get implementation handle (for internal use only by engine systems)
    virtual void* GetNativeHandle() const = 0;
};
