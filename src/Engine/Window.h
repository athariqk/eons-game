#pragma once

#include <cstdint>

struct SDL_Renderer;
struct SDL_Window;
struct Vector2D;

class Window {
public:
    Window(const char *title, int width, int height, bool fullscreen);
    ~Window();

    SDL_Renderer *GetRenderer() const;

    SDL_Window *GetWindow() const;

    Vector2D GetResolution() const;

    uint32_t GetSDLWindowID() const;

    void SetTitle(const char *title) const;

    int ShowMessageBox(uint32_t flags, const char *title, const char *message) const;

private:
    SDL_Window *sdlWindow{};
    SDL_Renderer *renderer{};
};
