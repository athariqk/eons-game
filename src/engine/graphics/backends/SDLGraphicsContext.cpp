#include "SDLGraphicsContext.h"

namespace ncore {

void SDLGraphicsContext::clear() { SDL_RenderClear(renderer); }

void SDLGraphicsContext::present() { SDL_RenderPresent(renderer); }

void SDLGraphicsContext::set_draw_color(const Color &color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void SDLGraphicsContext::draw_line(const Rect &rect) { SDL_RenderLine(renderer, rect.x, rect.y, rect.w, rect.h); }

void SDLGraphicsContext::draw_rect(const Rect &rect) {
    SDL_FRect sdlRect = {rect.x, rect.y, rect.w, rect.h};
    SDL_RenderRect(renderer, &sdlRect);
}

void SDLGraphicsContext::fill_rect(const Rect &rect) {
    SDL_FRect sdlRect = {rect.x, rect.y, rect.w, rect.h};
    SDL_RenderFillRect(renderer, &sdlRect);
}

void SDLGraphicsContext::draw_point(float x, float y) { SDL_RenderPoint(renderer, x, y); }

} // namespace ncore
