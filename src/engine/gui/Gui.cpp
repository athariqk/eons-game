#include "Gui.h"

#include <SDL3/SDL_events.h>

#include <EventBus.h>
#include <InputEvents.h>

#include "ImGuiSDL.h"
#include "ImGuiSDLRenderer.h"
#include "Logger.h"
#include "Window.h"

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
    LOG_INFO(log::GUI, "ImGui Initialized");
}

SDL_Scancode KeyToScancode(KeyboardEvent::Key key) {
    switch (key) {
        case KeyboardEvent::Key::W:
            return SDL_SCANCODE_W;
        case KeyboardEvent::Key::A:
            return SDL_SCANCODE_A;
        case KeyboardEvent::Key::S:
            return SDL_SCANCODE_S;
        case KeyboardEvent::Key::D:
            return SDL_SCANCODE_D;
        case KeyboardEvent::Key::Up:
            return SDL_SCANCODE_UP;
        case KeyboardEvent::Key::Down:
            return SDL_SCANCODE_DOWN;
        case KeyboardEvent::Key::Left:
            return SDL_SCANCODE_LEFT;
        case KeyboardEvent::Key::Right:
            return SDL_SCANCODE_RIGHT;
        case KeyboardEvent::Key::Space:
            return SDL_SCANCODE_SPACE;
        case KeyboardEvent::Key::Enter:
            return SDL_SCANCODE_RETURN;
        case KeyboardEvent::Key::Escape:
            return SDL_SCANCODE_ESCAPE;
        case KeyboardEvent::Key::Shift:
            return SDL_SCANCODE_LSHIFT;
        case KeyboardEvent::Key::Ctrl:
            return SDL_SCANCODE_LCTRL;
        case KeyboardEvent::Key::Alt:
            return SDL_SCANCODE_LALT;
        case KeyboardEvent::Key::Tab:
            return SDL_SCANCODE_TAB;
        case KeyboardEvent::Key::Backspace:
            return SDL_SCANCODE_BACKSPACE;
        default:
            return SDL_SCANCODE_UNKNOWN;
    }
}

uint8_t BtnToSDL(ButtonIndex btn) {
    switch (btn) {
        case ButtonIndex::Left:
            return SDL_BUTTON_LEFT;
        case ButtonIndex::Middle:
            return SDL_BUTTON_MIDDLE;
        case ButtonIndex::Right:
            return SDL_BUTTON_RIGHT;
        default:
            return 0;
    }
}

void Gui::init_event_subs(EventBus &event_bus) {
    window_id = m_window.get_window_id();
    auto &io = ImGui::GetIO();

    event_bus.subscribe<KeyboardEvent>([this, &io](const KeyboardEvent &e) {
        SDL_Event sdl{};
        sdl.type = e.action == ButtonAction::Press ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        sdl.key.scancode = KeyToScancode(e.key);
        sdl.key.key = 0;
        sdl.key.repeat = e.repeat;
        sdl.key.windowID = window_id;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
        if (io.WantCaptureKeyboard)
            e.handled = true;
    });

    event_bus.subscribe<MouseButtonEvent>([this, &io](const MouseButtonEvent &e) {
        SDL_Event sdl{};
        sdl.type = e.action == ButtonAction::Press ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
        sdl.button.button = BtnToSDL(e.button);
        sdl.button.x = e.position.x;
        sdl.button.y = e.position.y;
        sdl.button.windowID = window_id;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
        if (io.WantCaptureMouse)
            e.handled = true;
    });

    event_bus.subscribe<MouseMotionEvent>([this, &io](const MouseMotionEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_MOUSE_MOTION;
        sdl.motion.x = e.position.x;
        sdl.motion.y = e.position.y;
        sdl.motion.xrel = e.delta.x;
        sdl.motion.yrel = e.delta.y;
        sdl.motion.state = e.buttonState;
        sdl.motion.windowID = window_id;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
        if (io.WantCaptureMouse)
            e.handled = true;
    });

    event_bus.subscribe<MouseWheelEvent>([this, &io](const MouseWheelEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_MOUSE_WHEEL;
        sdl.wheel.x = e.scroll_x;
        sdl.wheel.y = e.scroll_y;
        sdl.wheel.windowID = window_id;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
        if (io.WantCaptureMouse)
            e.handled = true;
    });

    event_bus.subscribe<TextInputEvent>([this](const TextInputEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_TEXT_INPUT;
        sdl.text.text = e.text.c_str();
        sdl.text.windowID = window_id;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
        e.handled = true;
    });

    event_bus.subscribe<WindowResizeEvent>([this](const WindowResizeEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_WINDOW_RESIZED;
        sdl.window.data1 = e.width;
        sdl.window.data2 = e.height;
        sdl.window.windowID = window_id;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
    });

    event_bus.subscribe<WindowCloseEvent>([this](const WindowCloseEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        sdl.window.windowID = window_id;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
    });

    event_bus.subscribe<WindowFocusEvent>([this](const WindowFocusEvent &e) {
        SDL_Event sdl{};
        sdl.type = e.focused ? SDL_EVENT_WINDOW_FOCUS_GAINED : SDL_EVENT_WINDOW_FOCUS_LOST;
        sdl.window.windowID = window_id;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
    });

    event_bus.subscribe<WindowMouseEnterEvent>([this](const WindowMouseEnterEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_WINDOW_MOUSE_ENTER;
        sdl.window.windowID = window_id;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
    });

    event_bus.subscribe<WindowMouseLeaveEvent>([this](const WindowMouseLeaveEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_WINDOW_MOUSE_LEAVE;
        sdl.window.windowID = window_id;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
    });
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

