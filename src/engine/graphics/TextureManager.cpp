#include "TextureManager.h"

#include "Logger.h"

#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>

namespace ncore {

SDL_Texture *TextureManager::GetTexture(std::string filePath) {
    auto it = textureCache.find(filePath);
    if (it != textureCache.end()) {
        return it->second;
    }

    SDL_Texture *texture = IMG_LoadTexture(m_renderer, filePath.c_str());
    if (texture == nullptr) {
        LOG_ERROR(log::GRAPHICS, "Loading `{}` error: {}", filePath, SDL_GetError());
        return nullptr;
    }

    textureCache[filePath] = texture;
    return texture;
}

void TextureManager::DestroyTexture(SDL_Texture *texturePtr) {
    if (texturePtr == nullptr) {
        LOG_ERROR(log::GRAPHICS, "Texture is invalid: {}", SDL_GetError());
        return;
    }

    SDL_DestroyTexture(texturePtr);
}

void TextureManager::ClearCache() {
    for (auto &pair: textureCache) {
        if (pair.second) {
            DestroyTexture(pair.second);
        }
    }
    textureCache.clear();
}

void TextureManager::Draw(SDL_Renderer *renderer, SDL_Texture *tex, SDL_FRect *src, SDL_FRect *dest, double angle) {
    SDL_RenderTextureRotated(renderer, tex, src, dest, angle * RADIAN_TO_DEGREE, NULL, SDL_FlipMode::SDL_FLIP_NONE);
}

} // namespace ncore
