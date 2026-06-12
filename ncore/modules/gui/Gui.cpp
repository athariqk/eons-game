#include "Gui.h"

#include <SDL3/SDL_events.h>

#include <modules/events/Event.h>
#include <modules/events/EventBus.h>
#include <modules/events/InputEvent.h>
#include <modules/graphics/Window.h>
#include <modules/utils/Logger.h>
#include <platform/imgui/ImGuiSDL.h>
#include <platform/imgui/ImGuiSDLRenderer.h>
#include <platform/sdl3/events/EventBackendSDL.h>

namespace ncore {

Gui::Gui(Window &window) : m_window(window) {
    auto *renderer = window.get_renderer();
    if (!renderer) {
        LOG_ERROR(log::GUI, "Failed to initialize ImGui, SDL renderer is missing!");
        return;
    }

    IMGUI_CHECKVERSION();
    imgui_ctx = ImGui::CreateContext();

    if (!ImGui_ImplSDL3_InitForSDLRenderer(window.get_native_handle(), renderer)) {
        LOG_ERROR(log::GUI, "Failed to initialize ImGui SDL3 backend");
        ImGui::DestroyContext();
        return;
    }

    if (!ImGui_ImplSDLRenderer3_Init(renderer)) {
        LOG_ERROR(log::GUI, "Failed to initialize ImGui SDL renderer backend");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        return;
    }

    ImGui::StyleColorsLight();

    m_initialized = true;
    LOG_TRACE(log::GUI, "GUI OK");
}

void Gui::init_event_subs(EventBus &event_bus) {
    window_id = m_window.get_window_id();
    auto &io = ImGui::GetIO();

    auto forward = [this](WindowEvent &e, ImGuiIO &io) {
        auto sdl = EventBackendSDL::map_to_sdl(&e);
        ImGui_ImplSDL3_ProcessEvent(&sdl);
        if (io.WantCaptureKeyboard || io.WantCaptureMouse)
            e.handled = true;
    };

    event_bus.subscribe<KeyboardEvent>([&](KeyboardEvent &e) { forward(e, io); });
    event_bus.subscribe<MouseButtonEvent>([&](MouseButtonEvent &e) { forward(e, io); });
    event_bus.subscribe<MouseMotionEvent>([&](MouseMotionEvent &e) { forward(e, io); });
    event_bus.subscribe<MouseWheelEvent>([&](MouseWheelEvent &e) { forward(e, io); });
    event_bus.subscribe<TextInputEvent>([&](TextInputEvent &e) { forward(e, io); });
    event_bus.subscribe<WindowFocusEvent>([&](WindowFocusEvent &e) { forward(e, io); });
    event_bus.subscribe<WindowMouseEnterEvent>([&](WindowMouseEnterEvent &e) { forward(e, io); });
    event_bus.subscribe<WindowMouseLeaveEvent>([&](WindowMouseLeaveEvent &e) { forward(e, io); });
}

void Gui::begin() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();
}

void Gui::end() {
    if (!m_initialized) {
        return;
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_window.get_renderer());
}

void Gui::clear() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

} // namespace ncore
