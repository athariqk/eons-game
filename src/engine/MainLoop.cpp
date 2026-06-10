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

namespace ncore {

MainLoop::MainLoop(const std::string &p_app_name, Services &p_service_reg) :
    app_name(p_app_name), service_reg(p_service_reg) {
    auto &event_bus = get_event_bus();
    event_bus.subscribe<WindowCloseEvent>([this](const WindowCloseEvent &e) {
        is_running = false;
        LOG_TRACE(log::ENGINE, "Window close event received, stopping main loop...");
    });
    event_bus.subscribe<WindowResizeEvent>([this](const WindowResizeEvent &e) {
        service_reg.get<Viewport2D>().set_size(static_cast<float>(e.width), static_cast<float>(e.height));
        LOG_TRACE(log::ENGINE, "Window resolution changed: {} x {}", e.width, e.height);
    });
}

MainLoop::~MainLoop() = default;

int MainLoop::run() {
    // Start the game loop
    is_running = true;

    constexpr int fps = 60;
    constexpr int frame_delay = 1000 / fps;
    constexpr double FIXED_DT = 1.0 / 60.0; // Fixed timestep: 16.67ms
    constexpr double MAX_ACCUMULATOR = FIXED_DT * 5.0; // Prevent spiral of death

    double deltaTime = 0.0;
    double accumulator = 0.0;

    uint64_t last_time = SDL_GetPerformanceCounter();
    const double perf_freq = static_cast<double>(SDL_GetPerformanceFrequency());

    uint32_t last_fps_update_time = SDL_GetTicks();
    int frame_count = 0;

    while (is_running) {
        uint64_t cur_time = SDL_GetPerformanceCounter();
        double delta_time = static_cast<double>(cur_time - last_time) / perf_freq;
        last_time = cur_time;

        // Clamp to prevent freeze on lag spike
        if (delta_time > MAX_ACCUMULATOR) {
            delta_time = MAX_ACCUMULATOR;
        }

        if (is_world_dirty) {
            LOG_TRACE(log::ENGINE, "World change requested");
            update_active_world();
            is_world_dirty = false;
            LOG_TRACE(log::ENGINE, "World change complete");
        }

        event_bus.process_queue();
        poll_events();

        // Accumulate time
        accumulator += delta_time;

        // Deterministic timestep updates
        while (accumulator >= FIXED_DT) {
            fixed_update(FIXED_DT);
            accumulator -= FIXED_DT;
            ticks++;
        }

        // Variable update & render
        // Note: Can add interpolation here using (accumulator / FIXED_DT) as alpha
        update(delta_time);
        if (active_world)
            active_world->on_post_update_(delta_time);

        // FPS Counter (still fine to use GetTicks for a 1-second interval check)
        uint32_t cur_ticks = SDL_GetTicks();
        frame_count++;

        if (cur_ticks - last_fps_update_time > 1000) {
            float cur_fps = frame_count * 1000.0f / (cur_ticks - last_fps_update_time);
            frame_count = 0;
            last_fps_update_time = cur_ticks;

            std::array<char, 100> attrs{};
            std::snprintf(attrs.data(), attrs.size(), "FPS: %.1f - Delta: %.6f", cur_fps, delta_time);
            const std::string full_title = std::format("{} - {}", app_name, attrs.data());
            service_reg.get<Window>().set_title(full_title.c_str());
        }

        frame_count++;
    }

    // Teardown
    clean();
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

ButtonAction MapSDLEventTypeToAction(uint32_t p_sdl_event_type) {
    switch (p_sdl_event_type) {
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

ButtonIndex MapSDLButtonToButtonIndex(uint8_t p_sdl_button) {
    switch (p_sdl_button) {
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

void MainLoop::poll_events() {
    SDL_Event sdl_event;
    auto &event_bus = get_event_bus();

    while (SDL_PollEvent(&sdl_event)) {
        switch (sdl_event.type) {
            case SDL_EVENT_QUIT: {
                event_bus.queue(WindowCloseEvent());
                break;
            }

            case SDL_EVENT_WINDOW_RESIZED: {
                event_bus.queue(WindowResizeEvent(sdl_event.window.data1, sdl_event.window.data2));
                break;
            }

            case SDL_EVENT_WINDOW_MOUSE_ENTER: {
                event_bus.queue(WindowMouseEnterEvent());
                break;
            }

            case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
                event_bus.queue(WindowMouseLeaveEvent());
                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                event_bus.queue(WindowFocusEvent(sdl_event.type == SDL_EVENT_WINDOW_FOCUS_GAINED));
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                auto action = MapSDLEventTypeToAction(sdl_event.type);
                auto btn = MapSDLButtonToButtonIndex(sdl_event.button.button);
                auto mouse_pos = Vec2D(sdl_event.button.x, sdl_event.button.y);
                event_bus.queue(MouseButtonEvent(action, btn, mouse_pos));
                break;
            }

            case SDL_EVENT_MOUSE_MOTION: {
                auto mouse_pos = Vec2D(sdl_event.motion.x, sdl_event.motion.y);
                auto delta = Vec2D(sdl_event.motion.xrel, sdl_event.motion.yrel);
                auto state = sdl_event.motion.state;
                event_bus.queue(MouseMotionEvent(mouse_pos, delta, state));
                break;
            }

            case SDL_EVENT_MOUSE_WHEEL: {
                event_bus.queue(MouseWheelEvent(sdl_event.wheel.x, sdl_event.wheel.y));
                break;
            }

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                auto key = MapSDLKeyToKey(sdl_event.key.scancode);
                if (key != KeyboardEvent::Key::Unknown) {
                    auto action = MapSDLEventTypeToAction(sdl_event.type);
                    event_bus.queue(KeyboardEvent(action, key, sdl_event.key.repeat));
                }
                break;
            }

            case SDL_EVENT_TEXT_INPUT: {
                event_bus.queue(TextInputEvent(sdl_event.text.text));
                break;
            }

            default: {
                event_bus.queue(UnknownEvent(&sdl_event));
                break;
            }
        }
    }
}

void MainLoop::fixed_update(double p_delta) {
    if (active_world)
        active_world->on_fixed_update_(p_delta, ticks);
}

void MainLoop::update(double p_delta) {
    if (active_world)
        active_world->on_variable_update_(p_delta);

    // Below is for rendering the world and GUI.
    // World is rendered first, then GUI is drawn on top.

    auto viewport = get_services().try_get<Viewport2D>();
    if (!viewport) {
        LOG_ERROR(log::ENGINE, "No viewport available for rendering!");
        return;
    }

    auto graphics = viewport->get_graphics_context();
    if (!graphics) {
        LOG_ERROR(log::ENGINE, "No graphics context available for rendering!");
        return;
    }

    /* Solid background color (RGB: 70, 130, 180) */
    graphics->set_draw_color({0, 0, 0, 255});
    graphics->clear();

    if (active_world)
        active_world->on_render_(*graphics);

    auto gui = get_services().try_get<Gui>();
    if (!gui) {
        LOG_ERROR(log::ENGINE, "No ImGuiLayer service found for GUI rendering!");
        return;
    }

    gui->begin();

    ImGui::Begin("Main Loop", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::Button("Reset", ImVec2(100, 20))) {
        // HACK: SUPER HACKY STUFF GOIN ON HERE
        change_world(std::unique_ptr<MicrocosmWorld>(new MicrocosmWorld()));
    }
    ImGui::End();

    if (active_world)
        active_world->on_gui_render_();

    gui->end();

    graphics->present();
}

void MainLoop::clean() {
    if (active_world)
        active_world->on_finish_();
    event_bus.clear();
}

void MainLoop::change_world(std::unique_ptr<World> p_world) {
    is_world_dirty = true;
    next_world = std::move(p_world);
}

World &MainLoop::get_current_world() { return *active_world; }

void MainLoop::update_active_world() {
    // Teardown old
    if (active_world)
        active_world->on_finish_();

    // Actual change
    active_world = std::move(next_world);
    if (active_world) {
        active_world->set_main_loop(*this);
        active_world->on_init_();
    }
}

} // namespace ncore

