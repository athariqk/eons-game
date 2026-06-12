#include "SDLImageLoader.h"

#include <SDL3_image/SDL_image.h>
#include <utils/Structures.h>

namespace ncore {

Image SDLImageLoader::load_from_disk(std::string location) {
    SDL_Surface *surface = IMG_Load(location.c_str());
    if (!surface) {
        NC_LOG_ERROR_C(log::IO, "Failed to load image from path: {}. Error: {}", location, SDL_GetError());
        return {};
    }

    Image result;
    BytesBuffer buf(surface->w * surface->h * 4);
    SDL_ConvertPixels(surface->w, surface->h, surface->format, surface->pixels, surface->pitch, SDL_PIXELFORMAT_RGBA32,
                      buf.data(), surface->w * 4);
    result.data = std::move(buf);
    result.width = static_cast<float>(surface->w);
    result.height = static_cast<float>(surface->h);
    SDL_DestroySurface(surface);

    return result;
}

} // namespace ncore
