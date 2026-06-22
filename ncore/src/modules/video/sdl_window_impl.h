#pragma once

#include <ncore/modules/video/window_service.h>

struct SDL_Renderer;
struct SDL_Window;

namespace ncore {

struct Vec2;

class SDLWindowImpl : public IWindowService {
    NCLASS(SDLWindowImpl, IWindowService)

public:
    SDLWindowImpl(const char *title, int width, int height, bool fullscreen);

    Error init() override;
    void cleanup() override;

    SDL_Renderer *get_renderer() const;
    SDL_Window *get_native_handle() const;
    Vec2 get_resolution() const;
    uint32_t get_window_id() const;
    void set_title(const char *title) const;
    int show_msg_box(uint32_t flags, const char *title, const char *message) const;

    Viewport *get_viewport() const override;

private:
    std::string_view title;
    Vec2 resolution;
    bool fullscreen;
    SDL_Window *sdlWindow{};
    SDL_Renderer *renderer{};
    std::unique_ptr<Viewport> viewport{}; // TODO: properly implement later
};

} // namespace ncore
