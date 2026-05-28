#include "Window.h"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include "Logger.h"
#include "Vector2D.h"

Window::Window(const char *title, int width, int height, const bool fullscreen) {
    int flags = 0;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG_INFO("SDL initialized");
    } else {
        LOG_ERROR("SDL failed to initialize!");
        return;
    }

    if (fullscreen)
        flags = SDL_WINDOW_FULLSCREEN;

    sdlWindow = SDL_CreateWindow(title, width, height, flags | SDL_WINDOW_RESIZABLE);
    if (sdlWindow) {
        LOG_INFO("Created window with resolution: {} x {}, ID: {}", width, height, GetSDLWindowID());
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
}

Window::~Window() {
    LOG_INFO("Destroying window {}", GetSDLWindowID());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(sdlWindow);
}

SDL_Renderer *Window::GetRenderer() const { return renderer; }

SDL_Window *Window::GetWindow() const { return sdlWindow; }

Vector2D Window::GetResolution() const {
    int width, height;
    SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
    return Vector2D(width, height);
}

uint32_t Window::GetSDLWindowID() const { return SDL_GetWindowID(sdlWindow); }

void Window::SetTitle(const char *title) const { SDL_SetWindowTitle(GetWindow(), title); }

int Window::ShowMessageBox(const uint32_t flags, const char *title, const char *message) const {
    return SDL_ShowSimpleMessageBox(flags, title, message, sdlWindow);
}
