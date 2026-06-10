#include "MainLoop.h"

#include <array>
#include <format>
#include <stdexcept>

#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_timer.h>

#include <Engine.h>
#include <ErrorCodes.h>
#include <InputEvents.h>
#include <Logger.h>
#include <MicrocosmWorld.h>
#include <World.h>
#include <imgui.h>

namespace Aeon {

MainLoop::MainLoop(const std::string &p_appName, Services &p_serviceRegistry) :
    m_appName(p_appName), m_services(p_serviceRegistry) {
    auto &eventBus = GetEventBus();
    eventBus.Subscribe<WindowCloseEvent>([this](const WindowCloseEvent &e) {
        m_running = false;
        LOG_TRACE("Window close event received, stopping main loop...");
    });
    eventBus.Subscribe<WindowResizeEvent>([this](const WindowResizeEvent &e) {
        m_services.Get<Viewport2D>().SetSize(static_cast<float>(e.width), static_cast<float>(e.height));
        LOG_TRACE("Window resolution changed: {} x {}", e.width, e.height);
    });
}

MainLoop::~MainLoop() = default;

int MainLoop::Run() {
    // Start the game loop
    m_running = true;

    constexpr int fps = 60;
    constexpr int frameDelay = 1000 / fps;
    constexpr double FIXED_DT = 1.0 / 60.0; // Fixed timestep: 16.67ms
    constexpr double MAX_ACCUMULATOR = FIXED_DT * 5.0; // Prevent spiral of death

    double deltaTime = 0.0;
    double accumulator = 0.0;

    uint64_t lastTime = SDL_GetPerformanceCounter();
    const double performanceFrequency = static_cast<double>(SDL_GetPerformanceFrequency());

    uint32_t lastFPSUpdateTime = SDL_GetTicks();
    int frameCount = 0;

    while (m_running) {
        uint64_t currentTime = SDL_GetPerformanceCounter();
        double deltaTime = static_cast<double>(currentTime - lastTime) / performanceFrequency;
        lastTime = currentTime;

        // Clamp to prevent freeze on lag spike
        if (deltaTime > MAX_ACCUMULATOR) {
            deltaTime = MAX_ACCUMULATOR;
        }

        if (m_worldDirty) {
            LOG_TRACE("World change requested");
            UpdateActiveWorld();
            m_worldDirty = false;
            LOG_TRACE("World change complete");
        }

        m_eventBus.ProcessQueue();
        PollEvents();

        // Accumulate time
        accumulator += deltaTime;

        // Deterministic timestep updates
        while (accumulator >= FIXED_DT) {
            FixedUpdate(FIXED_DT);
            accumulator -= FIXED_DT;
            m_ticks++;
        }

        // Variable update & render
        // Note: Can add interpolation here using (accumulator / FIXED_DT) as alpha
        Update(deltaTime);
        if (m_activeWorld)
            m_activeWorld->_OnPostUpdate(deltaTime);

        // FPS Counter (still fine to use GetTicks for a 1-second interval check)
        uint32_t currentTicks = SDL_GetTicks();
        frameCount++;

        if (currentTicks - lastFPSUpdateTime > 1000) {
            float currentFPS = frameCount * 1000.0f / (currentTicks - lastFPSUpdateTime);
            frameCount = 0;
            lastFPSUpdateTime = currentTicks;

            std::array<char, 100> attrs{};
            std::snprintf(attrs.data(), attrs.size(), "FPS: %.1f - Delta: %.6f", currentFPS, deltaTime);
            const std::string fullTitle = std::format("{} - {}", m_appName, attrs.data());
            m_services.Get<Window>().SetTitle(fullTitle.c_str());
        }

        frameCount++;
    }

    // Teardown
    Clean();
    return ERROR_OK;
}

KeyboardEvent::Key MapSDLKeyToKey(SDL_Scancode scancode) {
    switch (scancode) {
        case SDL_SCANCODE_W:
            return KeyboardEvent::Key::W;
        case SDL_SCANCODE_A:
            return KeyboardEvent::Key::A;
        case SDL_SCANCODE_S:
            return KeyboardEvent::Key::S;
        case SDL_SCANCODE_D:
            return KeyboardEvent::Key::D;
        case SDL_SCANCODE_UP:
            return KeyboardEvent::Key::Up;
        case SDL_SCANCODE_DOWN:
            return KeyboardEvent::Key::Down;
        case SDL_SCANCODE_LEFT:
            return KeyboardEvent::Key::Left;
        case SDL_SCANCODE_RIGHT:
            return KeyboardEvent::Key::Right;
        case SDL_SCANCODE_SPACE:
            return KeyboardEvent::Key::Space;
        case SDL_SCANCODE_RETURN:
            return KeyboardEvent::Key::Enter;
        case SDL_SCANCODE_ESCAPE:
            return KeyboardEvent::Key::Escape;
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:
            return KeyboardEvent::Key::Shift;
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:
            return KeyboardEvent::Key::Ctrl;
        case SDL_SCANCODE_LALT:
        case SDL_SCANCODE_RALT:
            return KeyboardEvent::Key::Alt;
        case SDL_SCANCODE_TAB:
            return KeyboardEvent::Key::Tab;
        case SDL_SCANCODE_BACKSPACE:
            return KeyboardEvent::Key::Backspace;
        default:
            return KeyboardEvent::Key::Unknown;
    }
}

ButtonAction MapSDLEventTypeToAction(uint32_t sdlEventType) {
    switch (sdlEventType) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            return ButtonAction::Press;
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            return ButtonAction::Release;
        default:
            return ButtonAction::Unknown;
    }
}

ButtonIndex MapSDLButtonToButtonIndex(uint8_t sdlButton) {
    switch (sdlButton) {
        case SDL_BUTTON_LEFT:
            return ButtonIndex::Left;
        case SDL_BUTTON_MIDDLE:
            return ButtonIndex::Middle;
        case SDL_BUTTON_RIGHT:
            return ButtonIndex::Right;
        default:
            return ButtonIndex::Unknown;
    }
}

void MainLoop::PollEvents() {
    SDL_Event sdlEvent;
    auto &eventBus = GetEventBus();

    while (SDL_PollEvent(&sdlEvent)) {
        switch (sdlEvent.type) {
            case SDL_EVENT_QUIT: {
                eventBus.Queue(WindowCloseEvent());
                break;
            }

            case SDL_EVENT_WINDOW_RESIZED: {
                eventBus.Queue(WindowResizeEvent(sdlEvent.window.data1, sdlEvent.window.data2));
                break;
            }

            case SDL_EVENT_WINDOW_MOUSE_ENTER: {
                eventBus.Queue(WindowMouseEnterEvent());
                break;
            }

            case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
                eventBus.Queue(WindowMouseLeaveEvent());
                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                eventBus.Queue(WindowFocusEvent(sdlEvent.type == SDL_EVENT_WINDOW_FOCUS_GAINED));
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                auto action = MapSDLEventTypeToAction(sdlEvent.type);
                auto btn = MapSDLButtonToButtonIndex(sdlEvent.button.button);
                auto mousePos = Vector2D(sdlEvent.button.x, sdlEvent.button.y);
                eventBus.Queue(MouseButtonEvent(action, btn, mousePos));
                break;
            }

            case SDL_EVENT_MOUSE_MOTION: {
                auto mousePos = Vector2D(sdlEvent.motion.x, sdlEvent.motion.y);
                auto delta = Vector2D(sdlEvent.motion.xrel, sdlEvent.motion.yrel);
                auto state = sdlEvent.motion.state;
                eventBus.Queue(MouseMotionEvent(mousePos, delta, state));
                break;
            }

            case SDL_EVENT_MOUSE_WHEEL: {
                eventBus.Queue(MouseWheelEvent(sdlEvent.wheel.x, sdlEvent.wheel.y));
                break;
            }

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                auto key = MapSDLKeyToKey(sdlEvent.key.scancode);
                if (key != KeyboardEvent::Key::Unknown) {
                    auto action = MapSDLEventTypeToAction(sdlEvent.type);
                    eventBus.Queue(KeyboardEvent(action, key, sdlEvent.key.repeat));
                }
                break;
            }

            case SDL_EVENT_TEXT_INPUT: {
                eventBus.Queue(TextInputEvent(sdlEvent.text.text));
                break;
            }

            default: {
                eventBus.Queue(UnknownEvent(&sdlEvent));
                break;
            }
        }
    }
}

void MainLoop::FixedUpdate(double p_delta) {
    if (m_activeWorld)
        m_activeWorld->_OnFixedUpdate(p_delta, m_ticks);
}

void MainLoop::Update(double p_delta) {
    if (m_activeWorld)
        m_activeWorld->_OnVariableUpdate(p_delta);

    // Below is for rendering the world and GUI.
    // World is rendered first, then GUI is drawn on top.

    auto viewport = GetServices().TryGet<Viewport2D>();
    if (!viewport) {
        LOG_ERROR("No viewport available for rendering!");
        return;
    }

    auto graphics = viewport->GetGraphicsContext();
    if (!graphics) {
        LOG_ERROR("No graphics context available for rendering!");
        return;
    }

    /* Solid background color (RGB: 70, 130, 180) */
    graphics->SetDrawColor({0, 0, 0, 255});
    graphics->Clear();

    if (m_activeWorld)
        m_activeWorld->_OnRender(*graphics);

    auto gui = GetServices().TryGet<Gui>();
    if (!gui) {
        LOG_ERROR("No ImGuiLayer service found for GUI rendering!");
        return;
    }

    gui->Begin();

    ImGui::Begin("Main Loop", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::Button("Reset", ImVec2(100, 20))) {
        // HACK: SUPER HACKY STUFF GOIN ON HERE
        ChangeWorld(std::unique_ptr<MicrocosmWorld>(new MicrocosmWorld()));
    }
    ImGui::End();

    if (m_activeWorld)
        m_activeWorld->_OnGuiRender();

    gui->End();

    graphics->Present();
}

void MainLoop::Clean() {
    if (m_activeWorld)
        m_activeWorld->_OnFinish();
    m_eventBus.Clear();
}

void MainLoop::ChangeWorld(std::unique_ptr<World> p_world) {
    m_worldDirty = true;
    m_nextWorld = std::move(p_world);
}

World &MainLoop::GetCurrentWorld() { return *m_activeWorld; }

void MainLoop::UpdateActiveWorld() {
    // Teardown old
    if (m_activeWorld)
        m_activeWorld->_OnFinish();

    // Actual change
    m_activeWorld = std::move(m_nextWorld);
    if (m_activeWorld) {
        m_activeWorld->SetMainLoop(*this);
        m_activeWorld->_OnInit();
    }
}

} // namespace Aeon
