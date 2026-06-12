#include "MainLoop.h"

#include <array>
#include <format>
#include <stdexcept>

// TODO: remove SDL deps from this implementation

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>
#include <imgui.h>

#include <Engine.h>
#include <modules/events/Event.h>
#include <modules/events/EventBus.h>
#include <platform/sdl3/events/EventBackendSDL.h>
#include <utils/ErrorCodes.h>

namespace ncore {

MainLoop::MainLoop(const std::string &p_app_name, Services &p_service_reg) :
    app_name(p_app_name), service_reg(p_service_reg) {
    auto &event_bus = get_event_bus();
    event_bus.subscribe<QuitEvent>([this](QuitEvent &e) {
        is_running = false;
        NC_LOG_TRACE("Window close event received, stopping main loop...");
    });
    event_bus.subscribe<WindowResizeEvent>([this](WindowResizeEvent &e) {
        service_reg.get<Viewport2D>().set_size(static_cast<float>(e.width), static_cast<float>(e.height));
        NC_LOG_TRACE("Window resolution changed: {}x{}", e.width, e.height);
    });
    event_bus.subscribe<WorldChangeRequestEvent>([this](WorldChangeRequestEvent &e) {
        update_active_world(std::move(e.new_world));
        NC_LOG_TRACE("World change complete");
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

void MainLoop::poll_events() {
    SDL_Event sdl_event;
    auto &event_bus = get_event_bus();

    while (SDL_PollEvent(&sdl_event)) {
        auto event = EventBackendSDL::map_from_sdl(sdl_event);
        event_bus.enqueue(std::move(event));
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
    NC_ASSERT_RET(viewport, "Viewport2D service not found! Update skipped.");

    auto graphics = viewport->get_graphics_context();
    NC_ASSERT_RET(graphics, "Graphics context not found in viewport! Update skipped.");

    /* Solid background color (RGB: 70, 130, 180) */
    graphics->set_draw_color({0, 0, 0, 255});
    graphics->clear();

    if (active_world)
        active_world->on_render_(*graphics);

    auto gui = get_services().try_get<Gui>();
    NC_ASSERT_RET(gui, "Gui service not found! Update skipped.");

    gui->begin();
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
    NC_ASSERT_RET(p_world != nullptr, "Cannot change to null world!");
    event_bus.enqueue(std::make_unique<WorldChangeRequestEvent>(std::move(p_world)));
    NC_LOG_TRACE("World change requested");
}

World &MainLoop::get_current_world() { return *active_world; }

void MainLoop::update_active_world(std::unique_ptr<World> p_new_world) {
    // Teardown old
    if (active_world)
        active_world->on_finish_();

    // Actual change
    active_world = std::move(p_new_world);
    if (active_world) {
        active_world->set_main_loop(*this);
        active_world->on_init_();
    }
}

} // namespace ncore
