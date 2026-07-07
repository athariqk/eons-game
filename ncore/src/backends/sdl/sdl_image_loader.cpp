#include "sdl_image_loader.h"

#include <ncore/modules/video/resources/image.h>
#include <ncore/utils/log.h>

namespace nc {

bool SDLImageLoader::is_handling_extension( const std::string& ext )
{
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga";
}

Ref<IResource> SDLImageLoader::import( const std::string_view path )
{
    // TODO: integrate dedicated library for image loading
    SDL_Surface* surface = nullptr;
    // SDL_Surface *surface = IMG_Load(path.data());
    if (!surface) {
        NC_LOG_ERROR_C( log::IO, "Failed to import image from path: {}. Error: {}", path, SDL_GetError() );
        return {};
    }

    BytesBuffer buf( surface->w * surface->h * 4 );
    SDL_ConvertPixels(
        surface->w, surface->h, surface->format, surface->pixels, surface->pitch, SDL_PIXELFORMAT_RGBA32, buf.data(),
        surface->w * 4
    );
    auto result = Ref<Image>::create( surface->w, surface->h, buf.data() );
    SDL_DestroySurface( surface );

    return result.as<IResource>();
}

} // namespace nc
