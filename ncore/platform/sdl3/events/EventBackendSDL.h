#pragma once

#include <SDL3/SDL_events.h>

#include <modules/events/Event.h>

namespace ncore {

struct EventBackendSDL {
    static std::unique_ptr<Event> map_from_sdl(SDL_Event &event);
    static SDL_Event map_to_sdl(const WindowEvent *event);
};

} // namespace ncore
