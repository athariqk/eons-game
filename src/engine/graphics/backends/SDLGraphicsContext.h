#pragma once

#include <IGraphicsContext.h>
#include <SDL3/SDL_render.h>

namespace Aeon {

class SDLGraphicsContext : public IGraphicsContext {
public:
    SDLGraphicsContext(SDL_Renderer *renderer) : m_renderer(renderer) {}

    void Clear() override;
    void Present() override;
    void SetDrawColor(const Color &color) override;

	void DrawLine(const Rect &rect) override;
    void DrawRect(const Rect &rect) override;
    void FillRect(const Rect &rect) override;
    void DrawPoint(float x, float y) override;

    void *GetNativeHandle() const override { return m_renderer; }

    SDL_Renderer *GetSDLRenderer() const { return m_renderer; }

private:
    SDL_Renderer *m_renderer;
};

} // namespace Aeon
