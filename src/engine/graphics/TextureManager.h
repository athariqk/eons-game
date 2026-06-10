#pragma once

#include <string>
#include <unordered_map>

struct SDL_Texture;
struct SDL_FRect;
struct SDL_Renderer;

namespace Aeon {

class TextureManager {
public:
    TextureManager(SDL_Renderer *ren) : m_renderer(ren) {}
    ~TextureManager() { ClearCache(); }

    /**
     * @brief Gets a texture resource.
     *
     * This allocates texture to the graphics device on first load
     * and caches it for later use.
     * @todo Abstract away the SDL internals
     */
    SDL_Texture *GetTexture(std::string filePath);

    void DestroyTexture(SDL_Texture *texturePtr);

    void ClearCache();

    static void Draw(SDL_Renderer *renderer, SDL_Texture *tex, SDL_FRect *src, SDL_FRect *dest, double angle);

private:
    SDL_Renderer *m_renderer;
    std::unordered_map<std::string, SDL_Texture *> textureCache;
};

} // namespace Aeon
