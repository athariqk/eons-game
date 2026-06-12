#include "Window.h"

#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include "utils/Structures.h"

namespace ncore {

Window::Window(const char *title, int width, int height, const bool fullscreen) {
    int flags = 0;

    if (fullscreen)
        flags = SDL_WINDOW_FULLSCREEN;

    sdlWindow = SDL_CreateWindow(title, width, height, flags | SDL_WINDOW_RESIZABLE);
    if (sdlWindow) {
        NC_LOG_TRACE_C(log::GRAPHICS, "New SDL window: ({}x{}), ID: {}", width, height, get_window_id());
    } else {
        NC_LOG_ERROR_C(log::GRAPHICS, "SDL window creation failed!");
        return;
    }

    SDL_SetWindowPosition(sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(sdlWindow);

    renderer = SDL_CreateRenderer(sdlWindow, nullptr);
    if (renderer) {
        NC_LOG_TRACE_C(log::GRAPHICS, "New SDL renderer: {}", SDL_GetRendererName(renderer));
    } else {
        NC_LOG_ERROR_C(log::GRAPHICS, "SDL renderer creation failed!");
        SDL_DestroyWindow(sdlWindow);
        sdlWindow = nullptr;
        return;
    }
}

Window::~Window() {
    NC_LOG_TRACE_C(log::GRAPHICS, "Destroying SDL renderer: {}", SDL_GetRendererName(renderer));
    SDL_DestroyRenderer(renderer);
    NC_LOG_TRACE_C(log::GRAPHICS, "Destroying SDL window. ID {}", get_window_id());
    SDL_DestroyWindow(sdlWindow);
}

SDL_Renderer *Window::get_renderer() const { return renderer; }

SDL_Window *Window::get_native_handle() const { return sdlWindow; }

Vec2 Window::get_resolution() const {
    int width, height;
    SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
    return Vec2(width, height);
}

uint32_t Window::get_window_id() const { return SDL_GetWindowID(sdlWindow); }

void Window::set_title(const char *title) const { SDL_SetWindowTitle(get_native_handle(), title); }

int Window::show_msg_box(const uint32_t flags, const char *title, const char *message) const {
    return SDL_ShowSimpleMessageBox(flags, title, message, sdlWindow);
}

} // namespace ncore
