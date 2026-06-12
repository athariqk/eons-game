#include "SDLGraphicsContext.h"

#include <modules/resources/Resource.h>
#include <platform/sdl3/resources/SDLImageLoader.h>

namespace ncore {

void SDLGraphicsContext::clear() { SDL_RenderClear(renderer); }

void SDLGraphicsContext::present() { SDL_RenderPresent(renderer); }

void SDLGraphicsContext::set_draw_color(const Color &color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void SDLGraphicsContext::draw_line(const Vec4 &rect) { SDL_RenderLine(renderer, rect.x, rect.y, rect.w, rect.h); }

void SDLGraphicsContext::draw_rect(const Vec4 &rect) {
    SDL_FRect sdlRect = {rect.x, rect.y, rect.w, rect.h};
    SDL_RenderRect(renderer, &sdlRect);
}

void SDLGraphicsContext::fill_rect(const Vec4 &rect) {
    SDL_FRect sdlRect = {rect.x, rect.y, rect.w, rect.h};
    SDL_RenderFillRect(renderer, &sdlRect);
}

void SDLGraphicsContext::draw_point(float x, float y) { SDL_RenderPoint(renderer, x, y); }

void SDLGraphicsContext::draw_texture(const Image *img, const Vec4 &p_dest, const Vec4 &p_src, float angle,
                                      const Color &color) {
    auto src = SDL_FRect{p_src.x, p_src.y, p_src.w, p_src.h};
    auto dest = SDL_FRect{p_dest.x, p_dest.y, p_dest.w, p_dest.h};
    SDL_RenderTextureRotated(renderer, get_or_upload_img(img), p_src.is_zero() ? nullptr : &src,
                             p_dest.is_zero() ? nullptr : &dest, angle, nullptr, SDL_FLIP_NONE);
}

SDL_Texture *SDLGraphicsContext::get_or_upload_img(const Image *image) {
    auto it = texture_cache.find(image);
    if (it != texture_cache.end())
        return it->second;

    SDL_Surface *surface =
        SDL_CreateSurfaceFrom(image->width, image->height, SDL_PIXELFORMAT_RGBA32,
                              const_cast<uint8_t *>(image->data.data()), static_cast<int>(image->width) * 4);
    if (!surface)
        return nullptr;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (texture)
        texture_cache[image] = texture;

    NC_LOG_TRACE_C(log::GRAPHICS, "Cached texture for image at {} ({} bytes)", static_cast<const void *>(image),
                   image->get_size_in_bytes());

    return texture;
}

} // namespace ncore
