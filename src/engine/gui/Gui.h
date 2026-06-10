#pragma once

#include <cstdint>

union SDL_Event;

struct ImGuiContext;

namespace ncore {

class Window;
class EventBus;

/**
 * @brief Immediate-mode GUI implementation
 */
class Gui {
public:
    Gui(Window &window);
    ~Gui() {}

    void init_event_subs(EventBus &eventBus);
    void begin();
    void end();
    void clear();

private:
    bool m_initialized = false;
    Window &m_window;
    uint32_t window_id = 0;

    ImGuiContext *imgui_ctx = nullptr;
};

} // namespace ncore
