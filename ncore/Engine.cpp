#include "Engine.h"

#include <iostream>
#include <sstream>

#include <SDL3/SDL_init.h>

#include "platform/sdl3/resources/SDLAudioLoader.h"
#include "platform/sdl3/resources/SDLImageLoader.h"
#include "utils/ErrorCodes.h"
#include "utils/logger/Sink.h"

namespace ncore {

Engine::Engine(std::string p_app_name) : app_name(p_app_name), cfg_map(cfg_filename) {}

Engine::~Engine() {}

int Engine::init() {
    window_cfg = cfg_map.load<cfg::Window>();
    render_cfg = cfg_map.load<cfg::Render>();
    cfg_log = cfg_map.load<cfg::Log>();

    // Setup logging
    log::Logger::get_instance().add_sink(std::make_shared<log::FileSink>(cfg_log.FilePath));
    log::Logger::get_instance().set_level(log::Level(cfg_log.Level));
    if (!cfg_log.Overrides.empty()) {
        std::istringstream stream(cfg_log.Overrides);
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

    // TODO: engine is currently too coupled with SDL, gotta abstract all this out
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO)) {
        NC_LOG_TRACE("SDL init OK");
    } else {
        NC_LOG_ERROR("SDL init FAIL: {}", SDL_GetError());
        return ERROR_FATAL;
    }

    main_window =
        std::make_shared<Window>(app_name.c_str(), window_cfg.SizeWidth, window_cfg.SizeHeight, window_cfg.Fullscreen);
    viewport2d = std::make_shared<Viewport2D>(*main_window, render_cfg.PixelsPerMeter);
    phys2d = std::make_shared<Physics2D>();
    audio = std::make_shared<AudioManager>();
    gui = std::make_shared<Gui>(*main_window);
    res_mgr = std::make_shared<ResourceManager>();

    res_mgr->register_loader<Image>(SDLImageLoader());
    res_mgr->register_loader<AudioClip>(SDLAudioLoader());

    const auto resolution = main_window->get_resolution();
    viewport2d->set_rect({0.0f, 0.0f, resolution.x, resolution.y});

    // Register services
    service_reg.add<Window>(*main_window);
    service_reg.add<Physics2D>(*phys2d);
    service_reg.add<Viewport2D>(*viewport2d);
    service_reg.add<AudioManager>(*audio);
    service_reg.add<Gui>(*gui);
    service_reg.add<ResourceManager>(*res_mgr);

    main_loop = std::make_unique<MainLoop>(app_name, service_reg);
    gui->init_event_subs(main_loop->get_event_bus());

    return ERROR_OK;
}

int Engine::run(std::unique_ptr<World> p_default_world) {
    NC_ASSERT_RETVAL(init() == ERROR_OK, ERROR_FATAL, "Engine init FAIL!");
    main_loop->change_world(std::move(p_default_world));
    int result = main_loop->run();
    cleanup();
    return result;
}

int Engine::cleanup() {
    if (audio) {
        audio->clean();
    }

    if (gui) {
        gui->clear();
    }

    main_loop.reset();
    viewport2d.reset();
    main_window.reset();
    audio.reset();
    gui.reset();

    SDL_Quit();

    return ERROR_OK;
}

MainLoop *Engine::get_main_loop() {
    NC_ASSERT_RETVAL(main_loop, nullptr, "Main loop not set. Has this been initialized?");
    return main_loop.get();
}

} // namespace ncore
