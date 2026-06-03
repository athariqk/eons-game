#pragma once

#include <IGraphicsContext.h>
#include <SDL3/SDL_render.h>
#include <Window.h>
#include <memory>
#include "ICamera.h"
#include "Vector2D.h"

namespace Aeon {

//! \brief Defines the screen rectangle where rendering occurs
//! Also manages the main camera for view transformation
class Viewport2D {
public:
    Viewport2D(Window &p_window);
    Viewport2D(Window &p_window, float x, float y, float width, float height);
    ~Viewport2D();

    // Set/Get viewport rectangle
    void SetRect(float x, float y, float width, float height);
    void SetPosition(float x, float y);
    void SetSize(float width, float height);

    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetWidth() const { return m_width; }
    float GetHeight() const { return m_height; }

    Vector2D GetPosition() const { return Vector2D(m_x, m_y); }
    Vector2D GetSize() const { return Vector2D(m_width, m_height); }
    Vector2D GetCenter() const { return Vector2D(m_x + m_width / 2.0f, m_y + m_height / 2.0f); }

    ICamera *GetMainCamera() { return m_mainCamera.get(); }
    const ICamera *GetMainCamera() const { return m_mainCamera.get(); }
    void SetMainCamera(std::unique_ptr<ICamera> camera) { m_mainCamera = std::move(camera); }

    Vector2D WorldToScreen(const Vector2D &worldPos) const;
    Vector2D ScreenToWorld(const Vector2D &screenPos) const;

	float GetPixelsPerMeter() const { return m_pixelsPerMeter; }

    bool IsPointVisible(const Vector2D &worldPos) const;
    bool IsRectVisible(const Vector2D &worldPos, const Vector2D &size) const;

    // System access through interfaces
    IGraphicsContext *GetGraphicsContext() const { return m_graphicsContext.get(); }
    SDL_Window *GetSDLWindow() const { return m_window; } // HACK: For ImGui which needs SDL_Window

private:
    void Init(Window &p_window);

private:
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 0.0f;
    float m_height = 0.0f;
    float m_pixelsPerMeter = 32.0f; // 32 pixels = 1 meter, default scale factor for world-to-screen conversion

    std::unique_ptr<IGraphicsContext> m_graphicsContext;

    std::unique_ptr<ICamera> m_mainCamera;

    SDL_Window *m_window = nullptr; // HACK: Needed for ImGui initialization
};

} // namespace Aeon
