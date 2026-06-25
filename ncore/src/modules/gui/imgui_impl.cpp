#include "imgui_impl.h"

#include <modules/events/sdl_event_helpers.h>
#include <modules/gui/imgui_sdl.h>
#include <modules/gui/imgui_sdl_renderer.h>
#include <ncore/modules/events/event_bus.h>
#include <ncore/modules/events/events.h>
#include <ncore/modules/events/input_event.h>
#include <ncore/modules/service_locator.h>
#include <ncore/utils/log.h>

namespace ncore {

ImGuiImpl::ImGuiImpl(uint32_t p_window_id) : window_id(p_window_id) {}

Error ImGuiImpl::init()
{
    if (!window_id) {
        NC_LOG_ERROR(log::GUI, "Failed to initialize ImGui, SDL window is missing!");
        return Error::FAIL;
    }

    window   = SDL_GetWindowFromID(window_id);
    renderer = SDL_GetRenderer(window);

    IMGUI_CHECKVERSION();
    imgui_ctx = ImGui::CreateContext();

    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer)) {
        NC_LOG_ERROR(log::GUI, "Failed to initialize ImGui SDL3 backend");
        ImGui::DestroyContext();
        return Error::FAIL;
    }

    if (!ImGui_ImplSDLRenderer3_Init(renderer)) {
        NC_LOG_ERROR(log::GUI, "Failed to initialize ImGui SDL renderer backend");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        return Error::FAIL;
    }

    ImGui::StyleColorsClassic();

    m_initialized = true;
    NC_LOG_TRACE_C(log::GUI, "GUI OK");

    // now hooking up events

    auto& io = ImGui::GetIO();

    auto forward = [this](WindowEvent& e, ImGuiIO& io) {
        auto sdl = SDLEventHelpers::map_to_sdl(&e);
        ImGui_ImplSDL3_ProcessEvent(&sdl);
        if (io.WantCaptureKeyboard || io.WantCaptureMouse)
            e.handled = true;
    };

    auto event_bus = ServiceLocator::resolve<EventBus>();
    event_bus->subscribe<KeyboardEvent>([&](KeyboardEvent& e) { forward(e, io); });
    event_bus->subscribe<MouseButtonEvent>([&](MouseButtonEvent& e) { forward(e, io); });
    event_bus->subscribe<MouseMotionEvent>([&](MouseMotionEvent& e) { forward(e, io); });
    event_bus->subscribe<MouseWheelEvent>([&](MouseWheelEvent& e) { forward(e, io); });
    event_bus->subscribe<TextInputEvent>([&](TextInputEvent& e) { forward(e, io); });
    event_bus->subscribe<WindowFocusEvent>([&](WindowFocusEvent& e) { forward(e, io); });
    event_bus->subscribe<WindowMouseEnterEvent>([&](WindowMouseEnterEvent& e) { forward(e, io); });
    event_bus->subscribe<WindowMouseLeaveEvent>([&](WindowMouseLeaveEvent& e) { forward(e, io); });

    return Error::OK;
}

void ImGuiImpl::finalize()
{
    if (!m_initialized) {
        return;
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void ImGuiImpl::begin_frame()
{
    if (!m_initialized) {
        return;
    }

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiImpl::render_frame()
{
    if (!m_initialized) {
        return;
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

} // namespace ncore
