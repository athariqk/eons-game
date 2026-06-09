#include "SDLGraphicsContext.h"

namespace Aeon {

void SDLGraphicsContext::Clear() { SDL_RenderClear(m_renderer); }

void SDLGraphicsContext::Present() { SDL_RenderPresent(m_renderer); }

void SDLGraphicsContext::SetDrawColor(const Color &color) {
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
}

void SDLGraphicsContext::DrawLine(const Rect &rect) { SDL_RenderLine(m_renderer, rect.x, rect.y, rect.w, rect.h); }

void SDLGraphicsContext::DrawRect(const Rect &rect) {
    SDL_FRect sdlRect = {rect.x, rect.y, rect.w, rect.h};
    SDL_RenderRect(m_renderer, &sdlRect);
}

void SDLGraphicsContext::FillRect(const Rect &rect) {
    SDL_FRect sdlRect = {rect.x, rect.y, rect.w, rect.h};
    SDL_RenderFillRect(m_renderer, &sdlRect);
}

void SDLGraphicsContext::DrawPoint(float x, float y) { SDL_RenderPoint(m_renderer, x, y); }

} // namespace Aeon
