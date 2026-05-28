#include "ImGuiLayer.h"

#include <SDL3/SDL_render.h>

#include "ImGuiSDL.h"
#include "ImGuiSDLRenderer.h"
#include "Logger.h"

ImGuiLayer::ImGuiLayer(SDL_Window *window) {
    if (!window) {
        LOG_ERROR("Failed to initialize ImGui, window is missing!");
        return;
    }

    auto *renderer = SDL_GetRenderer(window);
    if (!renderer) {
        LOG_ERROR("Failed to initialize ImGui, SDL renderer is missing!");
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer)) {
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

    ImGui::StyleColorsClassic();

    m_initialized = true;
    LOG_INFO("ImGui Initialized");
}

void ImGuiLayer::OnEvent(const SDL_Event &event) {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplSDL3_ProcessEvent(&event);
}

void ImGuiLayer::Begin() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::End(SDL_Window *window) {
    if (!m_initialized || !window) {
        return;
    }

    auto *renderer = SDL_GetRenderer(window);
    if (!renderer) {
        return;
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

void ImGuiLayer::Clear() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}
