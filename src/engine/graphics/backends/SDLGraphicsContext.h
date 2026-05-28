#pragma once

#include <IGraphicsContext.h>
#include <SDL3/SDL_render.h>

class SDLGraphicsContext : public IGraphicsContext {
public:
    explicit SDLGraphicsContext(SDL_Renderer* renderer) : m_renderer(renderer) {}

    void Clear() override {
        SDL_RenderClear(m_renderer);
    }

    void Present() override {
        SDL_RenderPresent(m_renderer);
    }

    void SetDrawColor(const Color& color) override {
        SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    }

    void DrawRect(const Rect& rect) override {
        SDL_FRect sdlRect = {rect.x, rect.y, rect.w, rect.h};
        SDL_RenderRect(m_renderer, &sdlRect);
    }

    void FillRect(const Rect& rect) override {
        SDL_FRect sdlRect = {rect.x, rect.y, rect.w, rect.h};
        SDL_RenderFillRect(m_renderer, &sdlRect);
    }

    void DrawPoint(float x, float y) override {
        SDL_RenderPoint(m_renderer, x, y);
    }

    void* GetNativeHandle() const override {
        return m_renderer;
    }

    SDL_Renderer* GetSDLRenderer() const { return m_renderer; }

private:
    SDL_Renderer* m_renderer;
};
