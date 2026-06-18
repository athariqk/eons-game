#include "sdl_window_impl.h"

#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_render.h>

#include <kernel/structures.h>
#include <utils/macro.h>

namespace ncore {

SDLWindowImpl::SDLWindowImpl(const char *title, int width, int height, const bool fullscreen) :
    title(title), resolution(width, height), fullscreen(fullscreen) {
    int flags = 0;
    if (fullscreen)
        flags = SDL_WINDOW_FULLSCREEN;
    sdlWindow = SDL_CreateWindow(title, width, height, flags | SDL_WINDOW_RESIZABLE);
}

Error SDLWindowImpl::init() {
    if (sdlWindow) {
        NC_LOG_TRACE_C(log::GRAPHICS, "New SDL window: ({}x{}), ID: {}", resolution.x, resolution.y, get_window_id());
    } else {
        NC_LOG_ERROR_C(log::GRAPHICS, "SDL window creation failed!");
        return Error::FAIL;
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
        return Error::FAIL;
    }
    return Error::OK;
}

void SDLWindowImpl::cleanup() {
    NC_LOG_TRACE_C(log::GRAPHICS, "Destroying SDL renderer: {}", SDL_GetRendererName(renderer));
    SDL_DestroyRenderer(renderer);
    NC_LOG_TRACE_C(log::GRAPHICS, "Destroying SDL window. ID {}", get_window_id());
    SDL_DestroyWindow(sdlWindow);
}

SDL_Renderer *SDLWindowImpl::get_renderer() const { return renderer; }

SDL_Window *SDLWindowImpl::get_native_handle() const { return sdlWindow; }

Vec2 SDLWindowImpl::get_resolution() const {
    int width, height;
    SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
    return Vec2(width, height);
}

uint32_t SDLWindowImpl::get_window_id() const { return SDL_GetWindowID(sdlWindow); }

void SDLWindowImpl::set_title(const char *title) const { SDL_SetWindowTitle(get_native_handle(), title); }

int SDLWindowImpl::show_msg_box(const uint32_t flags, const char *title, const char *message) const {
    return SDL_ShowSimpleMessageBox(flags, title, message, sdlWindow);
}

} // namespace ncore
