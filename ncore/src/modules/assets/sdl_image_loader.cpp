#include "sdl_image_loader.h"

#include <utils/macro.h>

namespace ncore {

std::unique_ptr<Image> SDLImageLoader::load(const std::string_view path) {
    // TODO: integrate dedicated library for image loading
    SDL_Surface *surface = nullptr;
    // SDL_Surface *surface = IMG_Load(path.data());
    if (!surface) {
        NC_LOG_ERROR_C(log::IO, "Failed to load image from path: {}. Error: {}", path, SDL_GetError());
        return {};
    }

    auto result = std::make_unique<Image>();
    BytesBuffer buf(surface->w * surface->h * 4);
    SDL_ConvertPixels(surface->w, surface->h, surface->format, surface->pixels, surface->pitch, SDL_PIXELFORMAT_RGBA32,
                      buf.data(), surface->w * 4);
    result->data = std::move(buf);
    result->width = static_cast<float>(surface->w);
    result->height = static_cast<float>(surface->h);
    SDL_DestroySurface(surface);

    return result;
}

} // namespace ncore
