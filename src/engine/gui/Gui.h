#pragma once

#include <cstdint>

union SDL_Event;

struct ImGuiContext;

namespace Aeon {

class Window;
class EventBus;

/**
 * @brief Immediate-mode GUI implementation
 */
class Gui {
public:
    Gui(Window &window);
    ~Gui() {}

    void InitSubscriptions(EventBus &eventBus);
    void Begin();
    void End();
    void Clear();

private:
    bool m_initialized = false;
    Window &m_window;
    uint32_t m_windowID = 0;

    ImGuiContext *m_imguiContext = nullptr;
};

} // namespace Aeon
