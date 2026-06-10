#include "Window.h"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include "Logger.h"
#include "Vec2D.h"

namespace ncore {

Window::Window(const char *title, int width, int height, const bool fullscreen) {
    int flags = 0;

    if (fullscreen)
        flags = SDL_WINDOW_FULLSCREEN;

    sdlWindow = SDL_CreateWindow(title, width, height, flags | SDL_WINDOW_RESIZABLE);
    if (sdlWindow) {
        LOG_INFO(log::ENGINE, "Created window with resolution: {} x {}, ID: {}", width, height, get_window_id());
    } else {
        LOG_ERROR(log::ENGINE, "Failed to create window!");
        return;
    }

    SDL_SetWindowPosition(sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(sdlWindow);

    renderer = SDL_CreateRenderer(sdlWindow, nullptr);
    if (renderer) {
        LOG_INFO(log::ENGINE, "Created SDL Renderer.");
    } else {
        LOG_ERROR(log::ENGINE, "Failed to create SDL Renderer!");
        SDL_DestroyWindow(sdlWindow);
        sdlWindow = nullptr;
        return;
    }

    LOG_INFO(log::ENGINE, "SDL Renderer name: {}", SDL_GetRendererName(renderer));
}

Window::~Window() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(sdlWindow);
    LOG_INFO(log::ENGINE, "Destroyed window. ID {}", get_window_id());
}

SDL_Renderer *Window::get_renderer() const { return renderer; }

SDL_Window *Window::get_native_handle() const { return sdlWindow; }

Vec2D Window::get_resolution() const {
    int width, height;
    SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
    return Vec2D(width, height);
}

uint32_t Window::get_window_id() const { return SDL_GetWindowID(sdlWindow); }

void Window::set_title(const char *title) const { SDL_SetWindowTitle(get_native_handle(), title); }

int Window::show_msg_box(const uint32_t flags, const char *title, const char *message) const {
    return SDL_ShowSimpleMessageBox(flags, title, message, sdlWindow);
}

} // namespace ncore

