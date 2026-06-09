#include "Gui.h"

#include <SDL3/SDL_render.h>

#include "ImGuiSDL.h"
#include "ImGuiSDLRenderer.h"
#include "Logger.h"
#include "Window.h"

namespace Aeon {

Gui::Gui(Window &window) : m_window(window) {
    auto *renderer = window.GetRenderer();
    if (!renderer) {
        LOG_ERROR("Failed to initialize ImGui, SDL renderer is missing!");
        return;
    }

    IMGUI_CHECKVERSION();
    m_imguiContext = ImGui::CreateContext();

    if (!ImGui_ImplSDL3_InitForSDLRenderer(window.GetSDLWindow(), renderer)) {
        LOG_ERROR("Failed to initialize ImGui SDL3 backend");
        ImGui::DestroyContext();
        return;
    }

    if (!ImGui_ImplSDLRenderer3_Init(renderer)) {
        LOG_ERROR("Failed to initialize ImGui SDL renderer backend");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        return;
    }

    ImGui::StyleColorsLight();

    m_initialized = true;
    LOG_INFO("ImGui Initialized");
}

bool Gui::OnEvent(const SDL_Event &event) {
    if (!m_initialized) {
        LOG_ERROR("Received event for ImGui, but it is not initialized!");
        return false;
    }

    ImGui_ImplSDL3_ProcessEvent(&event);

    auto &io = ImGui::GetIO();
    return io.WantCaptureKeyboard || io.WantCaptureMouse;
}

void Gui::Begin() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();
}

void Gui::End() {
    if (!m_initialized) {
        return;
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_window.GetRenderer());
}

void Gui::Clear() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

} // namespace Aeon
