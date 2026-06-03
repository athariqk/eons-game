#include "TextureManager.h"

#include "Logger.h"

#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>

namespace Aeon {

SDL_Texture *TextureManager::LoadTexture(SDL_Renderer *renderer, const char *path) {
    SDL_Texture *texture = IMG_LoadTexture(renderer, path);
    if (texture == nullptr) {
        LOG_ERROR("Loading `{}` error: {}", path, SDL_GetError());
        return nullptr;
    }

    return texture;
}

void TextureManager::Draw(SDL_Renderer *renderer, SDL_Texture *tex, SDL_FRect *src, SDL_FRect *dest) {
    SDL_RenderTexture(renderer, tex, src, dest);
}

} // namespace Aeon
