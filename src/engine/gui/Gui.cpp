#include "Gui.h"

#include <SDL3/SDL_events.h>

#include <EventBus.h>
#include <InputEvents.h>

#include "ImGuiSDL.h"
#include "ImGuiSDLRenderer.h"
#include "Logger.h"
#include "Window.h"

namespace Aeon {

Gui::Gui(Window &window) : m_window(window) {
    auto *renderer = window.GetRenderer();
    if (!renderer) {
        LOG_ERROR(Log::Gui, "Failed to initialize ImGui, SDL renderer is missing!");
        return;
    }

    IMGUI_CHECKVERSION();
    m_imguiContext = ImGui::CreateContext();

    if (!ImGui_ImplSDL3_InitForSDLRenderer(window.GetSDLWindow(), renderer)) {
        LOG_ERROR(Log::Gui, "Failed to initialize ImGui SDL3 backend");
        ImGui::DestroyContext();
        return;
    }

    if (!ImGui_ImplSDLRenderer3_Init(renderer)) {
        LOG_ERROR(Log::Gui, "Failed to initialize ImGui SDL renderer backend");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        return;
    }

    ImGui::StyleColorsLight();

    m_initialized = true;
    LOG_INFO(Log::Gui, "ImGui Initialized");
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

void Gui::InitSubscriptions(EventBus &eventBus) {
    m_windowID = m_window.GetWindowID();
    auto &io = ImGui::GetIO();

    eventBus.Subscribe<KeyboardEvent>([this, &io](const KeyboardEvent &e) {
        SDL_Event sdl{};
        sdl.type = e.action == ButtonAction::Press ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        sdl.key.scancode = KeyToScancode(e.key);
        sdl.key.key = 0;
        sdl.key.repeat = e.repeat;
        sdl.key.windowID = m_windowID;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
        if (io.WantCaptureKeyboard)
            e.handled = true;
    });

    eventBus.Subscribe<MouseButtonEvent>([this, &io](const MouseButtonEvent &e) {
        SDL_Event sdl{};
        sdl.type = e.action == ButtonAction::Press ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
        sdl.button.button = BtnToSDL(e.button);
        sdl.button.x = e.position.x;
        sdl.button.y = e.position.y;
        sdl.button.windowID = m_windowID;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
        if (io.WantCaptureMouse)
            e.handled = true;
    });

    eventBus.Subscribe<MouseMotionEvent>([this, &io](const MouseMotionEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_MOUSE_MOTION;
        sdl.motion.x = e.position.x;
        sdl.motion.y = e.position.y;
        sdl.motion.xrel = e.delta.x;
        sdl.motion.yrel = e.delta.y;
        sdl.motion.state = e.buttonState;
        sdl.motion.windowID = m_windowID;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
        if (io.WantCaptureMouse)
            e.handled = true;
    });

    eventBus.Subscribe<MouseWheelEvent>([this, &io](const MouseWheelEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_MOUSE_WHEEL;
        sdl.wheel.x = e.scrollX;
        sdl.wheel.y = e.scrollY;
        sdl.wheel.windowID = m_windowID;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
        if (io.WantCaptureMouse)
            e.handled = true;
    });

    eventBus.Subscribe<TextInputEvent>([this](const TextInputEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_TEXT_INPUT;
        sdl.text.text = e.text.c_str();
        sdl.text.windowID = m_windowID;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
        e.handled = true;
    });

    eventBus.Subscribe<WindowResizeEvent>([this](const WindowResizeEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_WINDOW_RESIZED;
        sdl.window.data1 = e.width;
        sdl.window.data2 = e.height;
        sdl.window.windowID = m_windowID;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
    });

    eventBus.Subscribe<WindowCloseEvent>([this](const WindowCloseEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        sdl.window.windowID = m_windowID;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
    });

    eventBus.Subscribe<WindowFocusEvent>([this](const WindowFocusEvent &e) {
        SDL_Event sdl{};
        sdl.type = e.focused ? SDL_EVENT_WINDOW_FOCUS_GAINED : SDL_EVENT_WINDOW_FOCUS_LOST;
        sdl.window.windowID = m_windowID;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
    });

    eventBus.Subscribe<WindowMouseEnterEvent>([this](const WindowMouseEnterEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_WINDOW_MOUSE_ENTER;
        sdl.window.windowID = m_windowID;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
    });

    eventBus.Subscribe<WindowMouseLeaveEvent>([this](const WindowMouseLeaveEvent &e) {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_WINDOW_MOUSE_LEAVE;
        sdl.window.windowID = m_windowID;
        ImGui_ImplSDL3_ProcessEvent(&sdl);
    });
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

