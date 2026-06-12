#pragma once

#include <memory>
#include <string>

#include "modules/Config.h"
#include "modules/MainLoop.h"
#include "modules/Services.h"
#include "modules/audio/AudioManager.h"
#include "modules/graphics/Viewport.h"
#include "modules/graphics/Window.h"
#include "modules/gui/Gui.h"
#include "modules/physics/Physics.h"
#include "modules/resources/ResourceManager.h"

namespace ncore {

/**
 * @brief The main NCore engine class that initializes subsystems,
 * manages the main loop, and provides access to core services
 */
class Engine {
public:
    Engine(std::string p_app_name);
    ~Engine();

    int init();
    int run(std::unique_ptr<World> p_default_world);
    int cleanup();

    MainLoop *get_main_loop();

    cfg::Window window_cfg;
    cfg::Render render_cfg;
    cfg::Log cfg_log;

private:
    std::string app_name;
    std::string cfg_filename = "engine.ini";
    std::unique_ptr<MainLoop> main_loop;
    std::shared_ptr<Window> main_window;
    std::shared_ptr<Physics2D> phys2d;
    std::shared_ptr<Viewport2D> viewport2d;
    std::shared_ptr<AudioManager> audio;
    std::shared_ptr<Gui> gui;
    std::shared_ptr<ResourceManager> res_mgr;
    Services service_reg;
    ConfigMap cfg_map;
};

} // namespace ncore
