#pragma once

union SDL_Event;

struct ImGuiContext;

namespace Aeon {

class Window;

class Gui {
public:
    explicit Gui(Window &window);
    ~Gui() {}

    bool OnEvent(const SDL_Event &event);
    void Begin();
    void End();
    void Clear();

private:
    bool m_initialized = false;
    Window &m_window;

	ImGuiContext *m_imguiContext = nullptr;
};

} // namespace Aeon
