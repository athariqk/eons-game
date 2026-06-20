#include <ncore/application.h>

#include <array>
#include <chrono>
#include <format>
#include <memory>
#include <sstream>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

#include <ncore/kernel/types.h>
#include <ncore/modules/events/events.h>
#include <ncore/runtime/game_world.h>
#include <ncore/runtime/service_locator.h>
#include <ncore/utils/log.h>

#include <kernel/config.h>
#include <modules/assets/asset_manager.h>
#include <modules/assets/sdl_audio_loader.h>
#include <modules/assets/sdl_image_loader.h>
#include <modules/audio/sdl_audio_impl.h>
#include <modules/events/sdl_event_helpers.h>
#include <modules/gui/imgui_impl.h>
#include <modules/physics/box2d_physics_impl.h>
#include <modules/video/sdl_render_impl.h>
#include <modules/video/sdl_window_impl.h>
#include <modules/video/viewport.h>
#include <utils/logger/log_level.h>
#include <utils/logger/logger.h>
#include <utils/logger/sink.h>

namespace ncore {

namespace cfg {

struct Log {
    int Level = 0;
    std::string FilePath = "logs/engine.log";
    std::string Overrides;
    NSTRUCT(Log, NC_F(Log, Level) NC_F(Log, FilePath) NC_F(Log, Overrides));
};

struct Window {
    int SizeWidth = 800;
    int SizeHeight = 800;
    bool Fullscreen = false;
    NSTRUCT(Window, NC_F(Window, SizeWidth) NC_F(Window, SizeHeight) NC_F(Window, Fullscreen));
};

struct Render {
    float PixelsPerMeter = 32.0f;
    NSTRUCT(Render, NC_F(Render, PixelsPerMeter));
};

} // namespace cfg

Application::Application(const std::string &name, AppVersion version, const std::string &config_file) :
    app_name(name), app_version(version), config_file(config_file), services(ServiceLocator::get_instance()) {}

Application::~Application() { NC_ASSERT(!get_is_running(), "application destroyed while still running"); }

void Application::init() {
    auto cfg_file = ConfFile(config_file);
    auto log_cfg = cfg_file.read<cfg::Log>();
    auto window_cfg = cfg_file.read<cfg::Window>();
    auto render_cfg = cfg_file.read<cfg::Render>();

    // Set up logging
    log::Logger::get_instance().add_sink(std::make_shared<log::FileSink>(log_cfg.FilePath));
    log::Logger::get_instance().set_level(log::Level(log_cfg.Level));
    if (!log_cfg.Overrides.empty()) {
        std::istringstream stream(log_cfg.Overrides);
        std::string pair;
        while (std::getline(stream, pair, ',')) {
            auto sep = pair.find(':');
            if (sep != std::string::npos) {
                auto cat = pair.substr(0, sep);
                auto lvl = std::stoi(pair.substr(sep + 1));
                log::Logger::get_instance().set_level(cat, log::Level(lvl));
            }
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO)) {
        NC_LOG_ERROR("SDL init FAIL: {}", SDL_GetError());
        abort(); // TODO: handle this more gracefully
    }

    auto resources = services.provide<AssetManager>();
    resources->register_loader<Image>(SDLImageLoader());
    resources->register_loader<AudioClip>(SDLAudioLoader());

    auto window = services.provide<SDLWindowImpl>(app_name.c_str(), window_cfg.SizeWidth, window_cfg.SizeHeight,
                                                  window_cfg.Fullscreen);
    auto renderer = services.provide<SDLRenderImpl>(window->get_window_id());
    auto physics = services.provide<Box2DPhysicsImpl>();
    auto audio = services.provide<SDLAudioImpl>();
    auto gui = services.provide<ImGuiImpl>(window->get_window_id(), event_bus);

    services.init_all();

    auto viewport = std::make_unique<Viewport>(render_cfg.PixelsPerMeter);
    viewport->set_size(static_cast<float>(window_cfg.SizeWidth), static_cast<float>(window_cfg.SizeHeight));

    g_world = create_world();
    g_world->on_init();

    event_bus.subscribe<MainLoopStopEvent>([this](MainLoopStopEvent &) {
        is_running = false;
        NC_LOG_TRACE("stop event received, stopping main loop...");
    });

    event_bus.subscribe<WindowResizeEvent>(
        [this](WindowResizeEvent &e) { NC_LOG_TRACE("window resolution changed: {}x{}", e.width, e.height); });
}

void Application::run() {
    constexpr double FIXED_DT = 1.0 / 60.0;
    constexpr double MAX_ACCUMULATOR = FIXED_DT * 5.0;

    double accumulator = 0.0;
    auto last_time = std::chrono::high_resolution_clock::now();
    int frame_count = 0;
    auto last_fps_update_time = SDL_GetTicks();

    auto window = services.resolve<SDLWindowImpl>();
    std::array<char, 64> window_attrs;

    while (get_is_running()) {
        auto cur_time = std::chrono::high_resolution_clock::now();
        double delta_time = std::chrono::duration<double>(cur_time - last_time).count();
        last_time = cur_time;

        if (delta_time > MAX_ACCUMULATOR)
            delta_time = MAX_ACCUMULATOR;

        poll_events();

        accumulator += delta_time;
        while (accumulator >= FIXED_DT) {
            g_world->on_fixed_update(FIXED_DT, ticks);
            accumulator -= FIXED_DT;
            ticks++;
        }

        g_world->on_variable_update(delta_time);

        ticks = SDL_GetTicks();
        if (ticks - last_fps_update_time > 1000) {
            float fps = frame_count * 1000.0f / static_cast<float>(ticks - last_fps_update_time);
            frame_count = 0;
            last_fps_update_time = ticks;

            std::snprintf(window_attrs.data(), window_attrs.size(), "FPS: %.1f - Delta: %.6f", fps, delta_time);
            const std::string full_title = std::format("{} - {}", app_name, window_attrs.data());
            window->set_title(full_title.c_str());
        }

        frame_count++;
    }
}

void Application::poll_events() {
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event)) {
        auto event = SDLEventHelpers::map_from_sdl(sdl_event);
        event_bus.enqueue(std::move(event));
    }
}

void Application::finish() {
    g_world->on_finish();
    services.cleanup_all();
    event_bus.clear();
    SDL_Quit();
}

} // namespace ncore
