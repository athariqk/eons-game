#pragma once

#include <cstdint>

struct SDL_Renderer;
struct SDL_Window;

namespace ncore {

struct Vec2;

class Window {
public:
    Window(const char *title, int width, int height, bool fullscreen);
    ~Window();

    SDL_Renderer *get_renderer() const;

    SDL_Window *get_native_handle() const;

    Vec2 get_resolution() const;

    uint32_t get_window_id() const;

    void set_title(const char *title) const;

    int show_msg_box(uint32_t flags, const char *title, const char *message) const;

private:
    SDL_Window *sdlWindow{};
    SDL_Renderer *renderer{};
};

} // namespace ncore
