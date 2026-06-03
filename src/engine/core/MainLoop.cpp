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
#include <World.h>

namespace Aeon {

MainLoop::MainLoop(const std::string &p_appName, Services &p_serviceRegistry) :
    m_appName(p_appName), m_services(p_serviceRegistry) {}

MainLoop::~MainLoop() = default;

int MainLoop::Run() {
    if (m_worldDirty) {
        UpdateActiveWorld();
        m_worldDirty = false;
    }

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

void MainLoop::PollEvents() {
    if (!m_activeWorld)
        return;

    SDL_Event sdlEvent;
    auto &eventBus = GetEventBus();
    auto gui = GetServices().TryGet<Gui>();

    while (SDL_PollEvent(&sdlEvent)) {
        if (gui && gui->OnEvent(sdlEvent))
            continue; // If GUI handled the event, skip further processing

        switch (sdlEvent.type) {
            case SDL_EVENT_QUIT: {
                m_running = false;
                eventBus.Queue(WindowCloseEvent{});
                break;
            }

            case SDL_EVENT_WINDOW_RESIZED: {
                m_services.Get<Viewport2D>().SetSize(static_cast<float>(sdlEvent.window.data1),
                                                     static_cast<float>(sdlEvent.window.data2));
                eventBus.Queue(WindowResizeEvent{sdlEvent.window.data1, sdlEvent.window.data2});
                LOG_INFO("Window resolution changed: {} x {}", sdlEvent.window.data1, sdlEvent.window.data2);
                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                eventBus.Queue(WindowFocusEvent{sdlEvent.type == SDL_EVENT_WINDOW_FOCUS_GAINED});
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                // New event system
                eventBus.Queue(
                    MouseButtonEvent{sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? MouseButtonEvent::Action::Press
                                                                                  : MouseButtonEvent::Action::Release,
                                     sdlEvent.button.button, Vector2D(sdlEvent.button.x, sdlEvent.button.y)});
                break;
            }

            case SDL_EVENT_MOUSE_MOTION: {
                // New event system
                eventBus.Queue(MouseMotionEvent{Vector2D(sdlEvent.motion.x, sdlEvent.motion.y),
                                                Vector2D(sdlEvent.motion.xrel, sdlEvent.motion.yrel),
                                                sdlEvent.motion.state});
                break;
            }

            case SDL_EVENT_MOUSE_WHEEL: {
                eventBus.Queue(MouseWheelEvent{sdlEvent.wheel.x, sdlEvent.wheel.y});
                break;
            }

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                auto key = MapSDLKeyToKey(sdlEvent.key.scancode);
                if (key != KeyboardEvent::Key::Unknown) {
                    // New event system
                    auto action = sdlEvent.type == SDL_EVENT_KEY_DOWN ? KeyboardEvent::Action::Press
                                                                      : KeyboardEvent::Action::Release;
                    eventBus.Queue(KeyboardEvent{action, key, static_cast<bool>(sdlEvent.key.repeat)});
                }
                break;
            }

            default: {
                break;
            }
        }
    }
}

KeyboardEvent::Key MainLoop::MapSDLKeyToKey(SDL_Scancode scancode) {
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

void MainLoop::FixedUpdate(double p_delta) {
    m_eventBus.ProcessQueue();
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
    if (m_activeWorld)
        m_activeWorld->OnGuiRender();
    gui->End();

    graphics->Present();
}

void MainLoop::Clean() {
    if (m_activeWorld)
        m_activeWorld->_OnFinish();
    m_eventBus.Clear();
}

void MainLoop::ChangeWorld(std::unique_ptr<World> p_world) {
    LOG_TRACE("Requesting world change");
    m_worldDirty = true;
    m_nextWorld = std::move(p_world);
}

World &MainLoop::GetCurrentWorld() { return *m_activeWorld; }

void MainLoop::UpdateActiveWorld() {
    if (!m_nextWorld)
        return;

    if (m_activeWorld)
        m_activeWorld->_OnFinish();
    m_activeWorld = std::move(m_nextWorld);
    m_activeWorld->SetMainLoop(*this);
    LOG_TRACE("World change complete");

    m_activeWorld->_OnInit();
}

} // namespace Aeon
