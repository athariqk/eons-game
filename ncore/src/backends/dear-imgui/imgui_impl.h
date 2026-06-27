#pragma once

#include <ncore/modules/gui/gui_service.h>

struct ImGuiContext;

namespace ncore {

class SDLWindowImpl;
class EventBus;

/**
 * @brief Immediate-mode GUI implementation using Dear ImGui.
 */
class ImGuiImpl : public IImGuiService {
    NCLASS( ImGuiImpl, IImGuiService )

public:
    ImGuiImpl( uint32_t window_id );
    ~ImGuiImpl() {}

    Error init() override;
    void finalize() override;

    void begin_frame() override;
    void render_frame() override;

private:
    bool m_initialized      = false;
    uint32_t window_id      = 0;
    SDL_Renderer* renderer  = nullptr;
    SDL_Window* window      = nullptr;
    ImGuiContext* imgui_ctx = nullptr;
};

} // namespace ncore
