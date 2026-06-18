#include <ncore/runtime/game_loop.h>

#include <format>

#include <kernel/errors.h>

#include <modules/assets/asset_manager.h>
#include <modules/assets/sdl_audio_loader.h>
#include <modules/assets/sdl_image_loader.h>
#include <modules/audio/audio_clip.h>
#include <modules/audio/sdl_audio_impl.h>
#include <modules/ecs/ecs_world.h>
#include <modules/events/sdl_event_helpers.h>
#include <modules/gui/imgui_impl.h>
#include <modules/physics/box2d_physics_impl.h>
#include <modules/video/image.h>
#include <modules/video/sdl_render_impl.h>
#include <modules/video/sdl_window_impl.h>
#include <modules/video/viewport.h>
#include <runtime/service_locator.h>

namespace ncore {

Error GameLoop::init(IWorld &world) {
    auto window_cfg = cfg_file.read<cfg::Window>();
    auto render_cfg = cfg_file.read<cfg::Render>();

    // TODO: we are too coupled with SDL, consider abstraction or just entirely
    // replace it  later
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO)) {
        NC_LOG_ERROR("SDL init FAIL: {}", SDL_GetError());
        return Error::FATAL;
    }

    resources = services.provide<AssetManager>();
    resources->register_loader<Image>(SDLImageLoader());
    resources->register_loader<AudioClip>(SDLAudioLoader());

    window = services.provide<SDLWindowImpl>(app_name.c_str(), window_cfg.SizeWidth, window_cfg.SizeHeight,
                                             window_cfg.Fullscreen);
    renderer = services.provide<SDLRenderImpl>(window->get_window_id());
    physics = services.provide<Box2DPhysicsImpl>();
    audio = services.provide<SDLAudioImpl>();
    gui = services.provide<ImGuiImpl>(window->get_window_id(), event_bus);

    services.init_all();

    // Create viewport and set on world
    auto &ecs_world = static_cast<EcsWorld &>(world);
    viewport = std::make_unique<Viewport>(render_cfg.PixelsPerMeter);
    viewport->set_size(static_cast<float>(window_cfg.SizeWidth), static_cast<float>(window_cfg.SizeHeight));
    ecs_world.set_services(&services);
    ecs_world.set_viewport(viewport.get());
    ecs_world.set_event_bus(&event_bus);

    event_bus.subscribe<WindowResizeEvent>(
        [this](WindowResizeEvent &e) { NC_LOG_TRACE("window resolution changed: {}x{}", e.width, e.height); });

    return Error::OK;
}

Error GameLoop::main_loop() {
    constexpr double FIXED_DT = 1.0 / 60.0;
    constexpr double MAX_ACCUMULATOR = FIXED_DT * 5.0;

    double accumulator = 0.0;

    auto last_time = std::chrono::high_resolution_clock::now();
    int frame_count = 0;
    auto last_fps_update_time = SDL_GetTicks();

    while (get_is_running()) {
        auto cur_time = std::chrono::high_resolution_clock::now();
        double delta_time = std::chrono::duration<double>(cur_time - last_time).count();
        last_time = cur_time;

        if (delta_time > MAX_ACCUMULATOR)
            delta_time = MAX_ACCUMULATOR;

        event_bus.process_queue();
        poll_events();

        accumulator += delta_time;

        while (accumulator >= FIXED_DT) {
            get_world().on_fixed_update(FIXED_DT, ticks);
            accumulator -= FIXED_DT;
            ticks++;
        }

        renderer->set_draw_color({0, 0, 0, 255});
        // stub
        renderer->clear();

        gui->begin();
        // stub
        gui->end();

        renderer->present();

        get_world().on_post_update(delta_time);

        frame_count++;

        cur_ticks = SDL_GetTicks();
        if (cur_ticks - last_fps_update_time > 1000) {
            float fps = frame_count * 1000.0f / static_cast<float>(cur_ticks - last_fps_update_time);
            frame_count = 0;
            last_fps_update_time = cur_ticks;

            std::snprintf(attrs.data(), attrs.size(), "FPS: %.1f - Delta: %.6f", fps, delta_time);
            const std::string full_title = std::format("{} - {}", app_name, attrs.data());
            window->set_title(full_title.c_str());
        }
    }

    get_world().on_finish();
    event_bus.clear();

    return Error::OK;
}

Error GameLoop::poll_events() {
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event)) {
        auto event = SDLEventHelpers::map_from_sdl(sdl_event);
        event_bus.enqueue(std::move(event));
    }
    return Error::OK;
}

Error GameLoop::cleanup() {
    NC_ASSERT(get_is_initialized(), "cleanup called before init!");
    stop();
    main_loop();
    services.cleanup_all();
    SDL_Quit();
    return Error::OK;
}

void GameLoop::log(const std::string &message) {
    log::Logger::get_instance().channel()->write(log::Level::Info, log::SourceLoc(), "{}", message);
}

void GameLoop::log_error(const std::string &message) {
    log::Logger::get_instance().channel()->write(log::Level::Error, log::SourceLoc(), "{}", message);
}

} // namespace ncore
