#pragma once
#include <modules/gui/gui_service.h>

struct ImGuiContext;

namespace ncore {

class SDLWindowImpl;
class EventBus;

/**
 * @brief Immediate-mode GUI implementation
 */
class ImGuiImpl : public IIMGuiService {
    NCLASS(ImGuiImpl, IIMGuiService)

public:
    ImGuiImpl(uint32_t window_id);
    ~ImGuiImpl() {}

    Error init() override;
    void cleanup() override;

    void begin() override;
    void end() override;

private:
    bool m_initialized = false;
    uint32_t window_id = 0;
    SDL_Renderer *renderer = nullptr;
    SDL_Window *window = nullptr;
    ImGuiContext *imgui_ctx = nullptr;
};

} // namespace ncore
