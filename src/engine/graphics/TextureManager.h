#pragma once

struct SDL_Texture;
struct SDL_FRect;
struct SDL_Renderer;

class TextureManager {
public:
    static SDL_Texture *LoadTexture(SDL_Renderer *renderer, const char *path);

    static void Draw(SDL_Renderer *renderer, SDL_Texture *tex, SDL_FRect *src, SDL_FRect *dest);
};
