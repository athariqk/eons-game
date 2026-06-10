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
#include <utils/Config.h>

namespace Aeon {

namespace Config {

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

} // namespace Config

class Engine {
public:
    Engine(std::string appName);
    ~Engine();

    int Run();

    MainLoop &GetMainLoop() { return *m_mainLoop; }

    Config::Window m_windowConf;
    Config::Render m_renderConf;

private:
    int Init();
    int Cleanup();

private:
    std::string m_app_name;
    std::string m_configFileName = "engine.ini";
    ConfigMap m_config;

    std::unique_ptr<MainLoop> m_mainLoop;

private:
    // Engine subsystems
    std::shared_ptr<Window> m_mainWindow;
    std::shared_ptr<Physics2D> m_physics2d;
    std::shared_ptr<Viewport2D> m_viewport2d;
    std::shared_ptr<AudioManager> m_audio;
    std::shared_ptr<Gui> m_gui;
    std::shared_ptr<TextureManager> m_textureManager;

    // Service registry
    Services m_services;
};

} // namespace Aeon
