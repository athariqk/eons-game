#include "Window.h"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include "Logger.h"
#include "Vector2D.h"

namespace Aeon {

Window::Window(const char *title, int width, int height, const bool fullscreen) {
    int flags = 0;

    if (fullscreen)
        flags = SDL_WINDOW_FULLSCREEN;

    sdlWindow = SDL_CreateWindow(title, width, height, flags | SDL_WINDOW_RESIZABLE);
    if (sdlWindow) {
        LOG_INFO("Created window with resolution: {} x {}, ID: {}", width, height, GetWindowID());
    } else {
        LOG_ERROR("Failed to create window!");
        return;
    }

    SDL_SetWindowPosition(sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(sdlWindow);

    renderer = SDL_CreateRenderer(sdlWindow, nullptr);
    if (renderer) {
        LOG_INFO("Created SDL Renderer.");
    } else {
        LOG_ERROR("Failed to create SDL Renderer!");
        SDL_DestroyWindow(sdlWindow);
        sdlWindow = nullptr;
        return;
    }

    LOG_INFO("SDL Renderer name: {}", SDL_GetRendererName(renderer));
}

Window::~Window() {
    LOG_INFO("Destroying window {}", GetWindowID());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(sdlWindow);
}

SDL_Renderer *Window::GetRenderer() const { return renderer; }

SDL_Window *Window::GetSDLWindow() const { return sdlWindow; }

Vector2D Window::GetResolution() const {
    int width, height;
    SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
    return Vector2D(width, height);
}

uint32_t Window::GetWindowID() const { return SDL_GetWindowID(sdlWindow); }

void Window::SetTitle(const char *title) const { SDL_SetWindowTitle(GetSDLWindow(), title); }

int Window::ShowMessageBox(const uint32_t flags, const char *title, const char *message) const {
    return SDL_ShowSimpleMessageBox(flags, title, message, sdlWindow);
}

} // namespace Aeon
