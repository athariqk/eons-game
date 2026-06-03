#pragma once

struct SDL_Texture;
struct SDL_FRect;
struct SDL_Renderer;

namespace Aeon {

class TextureManager {
public:
    /**
	* @brief Allocates texture to the graphics device
	* @todo Abstract away the SDL internals
	*/
    static SDL_Texture *LoadTexture(SDL_Renderer *renderer, const char *path);

    static void Draw(SDL_Renderer *renderer, SDL_Texture *tex, SDL_FRect *src, SDL_FRect *dest);
};

} // namespace Aeon
