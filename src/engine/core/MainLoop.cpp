#include "MainLoop.h"

#include <Event.h>
#include <Logger.h>
#include <SDL3/SDL_scancode.h>
#include <SDLGraphicsContext.h>

MainLoop::MainLoop(Window &p_window) : m_mainWindow(p_window), m_viewport2d(p_window) {
    // Initialize viewport to full window
    auto resolution = p_window.GetResolution();
    m_viewport2d.SetRect(0, 0, resolution.x, resolution.y);
}

MainLoop::~MainLoop() = default;

void MainLoop::SetCurrentScene(std::unique_ptr<Scene> scene) {
    if (m_currentScene)
        m_currentScene->OnFinish();
    m_currentScene = std::move(scene);
    if (m_currentScene) {
        m_currentScene->SetPhysics2D(&m_physics2d);
        m_currentScene->SetAudioManager(&m_audio);
        m_currentScene->SetViewport(&m_viewport2d);
        m_currentScene->OnInit();
    }
}

Scene *MainLoop::GetCurrentScene() { return m_currentScene.get(); }

void MainLoop::Init() {
    // Start the game loop
    m_running = true;
}

void MainLoop::HandleEvents() {
    SDL_Event sdlEvent;

    while (SDL_PollEvent(&sdlEvent)) {
        std::shared_ptr<BaseEvent> event = nullptr;

        switch (sdlEvent.type) {
            case SDL_EVENT_QUIT: {
                m_running = false;
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED: {
                SDL_SetWindowSize(m_mainWindow.GetWindow(), sdlEvent.window.data1, sdlEvent.window.data2);
                LOG_INFO("Window resolution changed: {} x {}", sdlEvent.window.data1, sdlEvent.window.data2);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                auto mouseEvent = std::make_shared<MouseEvent>();
                mouseEvent->m_mouseButton = sdlEvent.button.button;
                mouseEvent->m_isPanning = (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
                mouseEvent->m_relativeMotion = Vector2D(0.0f, 0.0f);
                mouseEvent->m_handle = sdlEvent;
                event = mouseEvent;
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                auto mouseEvent = std::make_shared<MouseEvent>();
                // For motion events, check which buttons are currently pressed
                mouseEvent->m_mouseButton = sdlEvent.motion.state;
                mouseEvent->m_isPanning = (sdlEvent.motion.state != 0); // true if any button pressed
                mouseEvent->m_relativeMotion = Vector2D(sdlEvent.motion.xrel, sdlEvent.motion.yrel);
                mouseEvent->m_handle = sdlEvent;
                event = mouseEvent;
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                auto keyEvent = std::make_shared<KeyEvent>();
                keyEvent->m_type =
                    (sdlEvent.type == SDL_EVENT_KEY_DOWN) ? KeyEvent::Type::KeyDown : KeyEvent::Type::KeyUp;
                keyEvent->m_repeat = sdlEvent.key.repeat;
                keyEvent->m_key = MapSDLKeyToKey(sdlEvent.key.scancode);
                keyEvent->m_handle = sdlEvent;

                if (keyEvent->m_key != KeyEvent::Key::Unknown) {
                    event = keyEvent;
                }
                break;
            }
            default: {
                break;
            }
        }

        if (m_currentScene && event) {
            LOG_DEBUG("[MainLoop::HandleEvents] Dispatching {}", event->ToString());
            m_currentScene->OnEvent(event);
        }
    }
}

KeyEvent::Key MainLoop::MapSDLKeyToKey(SDL_Scancode scancode) {
    switch (scancode) {
        case SDL_SCANCODE_W:
            return KeyEvent::Key::W;
        case SDL_SCANCODE_A:
            return KeyEvent::Key::A;
        case SDL_SCANCODE_S:
            return KeyEvent::Key::S;
        case SDL_SCANCODE_D:
            return KeyEvent::Key::D;
        case SDL_SCANCODE_UP:
            return KeyEvent::Key::Up;
        case SDL_SCANCODE_DOWN:
            return KeyEvent::Key::Down;
        case SDL_SCANCODE_LEFT:
            return KeyEvent::Key::Left;
        case SDL_SCANCODE_RIGHT:
            return KeyEvent::Key::Right;
        case SDL_SCANCODE_SPACE:
            return KeyEvent::Key::Space;
        case SDL_SCANCODE_RETURN:
            return KeyEvent::Key::Enter;
        case SDL_SCANCODE_ESCAPE:
            return KeyEvent::Key::Escape;
        default:
            return KeyEvent::Key::Unknown;
    }
}

void MainLoop::Update(double p_delta, uint64_t p_ticks) {
    m_physics2d.Step();

    if (m_currentScene)
        m_currentScene->_OnUpdate(p_delta, p_ticks);

    m_ticks = p_ticks;
}

void MainLoop::Render() {
    auto ctx = m_viewport2d.GetGraphicsContext();

    /* Solid background color (RGB: 70, 130, 180) */
    ctx->SetDrawColor({70, 130, 180, 255});
    ctx->Clear();

    if (m_currentScene)
        m_currentScene->_OnRender();

    ctx->Present();
}

void MainLoop::Clean() {
    if (m_currentScene)
        m_currentScene->OnFinish();

    m_audio.Clear();

    SDL_Quit();
}
