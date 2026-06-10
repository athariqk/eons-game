#pragma once

#include <memory>
#include <string>

#include <AudioManager.h>
#include <Gui.h>
#include <MainLoop.h>
#include <Physics.h>
#include <Services.h>
#include <TextureManager.h>
#include <Viewport.h>
#include <Window.h>
#include <Config.h>

namespace ncore {

namespace cfg {

/**
 * @brief Properties related to hardware window settings
 */
struct Window {
    int SizeWidth = 800;
    int SizeHeight = 800;
    bool Fullscreen = false;
    DEFINE_CONFIG_MAP_FIELDS(Window, SizeWidth, SizeHeight, Fullscreen)
};

struct Render {
    float PixelsPerMeter = 32.0f;
    DEFINE_CONFIG_MAP_FIELDS(Render, PixelsPerMeter)
};

/**
 * @brief Properties related to the engine's runtime
 */
struct Runtime {
    std::string DefaultWorld;
    DEFINE_CONFIG_MAP_FIELDS(Runtime, DefaultWorld)
};

struct Log {
    std::string Level = "trace";
    std::string FilePath = "logs/engine.log";
    std::string Overrides; // e.g. "Physics:info,ECS:debug"
    DEFINE_CONFIG_MAP_FIELDS(Log, Level, FilePath, Overrides)
};

} // namespace cfg

class Engine {
public:
    Engine(std::string appName);
    ~Engine();

    int run();

    MainLoop &get_main_loop() { return *main_loop; }

    cfg::Window window_cfg;
    cfg::Render render_cfg;

private:
    int init();
    int cleanup();

private:
    std::string app_name;
    std::string cfg_filename = "engine.ini";
    ConfigMap cfg_map;

    std::unique_ptr<MainLoop> main_loop;

private:
    // Engine subsystems
    std::shared_ptr<Window> main_window;
    std::shared_ptr<Physics2D> phys2d;
    std::shared_ptr<Viewport2D> viewport2d;
    std::shared_ptr<AudioManager> audio;
    std::shared_ptr<Gui> gui;
    std::shared_ptr<TextureManager> texture;

    // Service registry
    Services service_reg;
};

} // namespace ncore
