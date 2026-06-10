#include "Engine.h"

#include <iostream>
#include <sstream>

#include <ErrorCodes.h>
#include <Logger.h>
#include <SDLGraphicsContext.h>

namespace ncore {

Engine::Engine(std::string p_app_name) : app_name(p_app_name), cfg_map(cfg_filename) { init(); }

Engine::~Engine() {}

int Engine::init() {
    window_cfg = cfg_map.load<cfg::Window>();
    render_cfg = cfg_map.load<cfg::Render>();
    auto logConf = cfg_map.load<cfg::Log>();
    if (auto err = Logger::init(logConf.FilePath)) {
        std::cerr << "log initialization failed: " << err.value() << std::endl;
    } else {
        LOG_INFO(log::ENGINE, "log system initialized");
    }
    Logger::set_level(log::ENGINE, logConf.Level);
    if (!logConf.Overrides.empty()) {
        std::istringstream stream(logConf.Overrides);
        std::string pair;
        while (std::getline(stream, pair, ',')) {
            auto sep = pair.find(':');
            if (sep != std::string::npos) {
                auto cat = pair.substr(0, sep);
                auto lvl = pair.substr(sep + 1);
                Logger::set_level(cat, lvl);
            }
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG_INFO(log::ENGINE, "SDL initialized");
    } else {
        LOG_ERROR(log::ENGINE, "SDL failed to initialize!");
        return ERROR_FATAL;
    }

    main_window =
        std::make_shared<Window>(app_name.c_str(), window_cfg.SizeWidth, window_cfg.SizeHeight, window_cfg.Fullscreen);
    viewport2d = std::make_shared<Viewport2D>(*main_window, render_cfg.PixelsPerMeter);
    phys2d = std::make_shared<Physics2D>();
    audio = std::make_shared<AudioManager>();
    gui = std::make_shared<Gui>(*main_window);
    auto renderer = dynamic_cast<SDLGraphicsContext *>(viewport2d->get_graphics_context());
    texture = std::make_shared<TextureManager>(renderer->get_native_handle());

    const auto resolution = main_window->get_resolution();
    viewport2d->set_rect({0.0f, 0.0f, resolution.x, resolution.y});

    // Register services
    service_reg.add<Window>(*main_window);
    service_reg.add<Physics2D>(*phys2d);
    service_reg.add<Viewport2D>(*viewport2d);
    service_reg.add<AudioManager>(*audio);
    service_reg.add<Gui>(*gui);
    service_reg.add<TextureManager>(*texture);

    main_loop = std::make_unique<MainLoop>(app_name, service_reg);
    gui->init_event_subs(main_loop->get_event_bus());

    return ERROR_OK;
}

int Engine::run() {
    if (!main_loop) {
        LOG_ERROR(log::ENGINE, "MainLoop is not initialized!");
        return ERROR_FATAL;
    }

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

} // namespace ncore
