#pragma once
#pragma once

#include <memory>
#include <string>

#include <ini.h>

#include <AudioManager.h>
#include <MainLoop.h>
#include <Physics.h>
#include <Services.h>
#include <Viewport.h>
#include <Window.h>
#include <Gui.h>

namespace Aeon {

class Engine {
public:
    Engine(std::string appName);
    ~Engine();

    int Run();

    int GetWindowSizeWidth();
    int GetWindowSizeHeight();
    bool GetWindowFullscreen();

    MainLoop &GetMainLoop() { return *m_mainLoop; }

private:
    int LoadConfiguration();
    int Init();
    int Cleanup();

private:
    std::string m_app_name;
    std::string m_configFileName = "engine.ini";
    mINI::INIStructure m_config{};

    std::unique_ptr<MainLoop> m_mainLoop;

private:
    // Engine subsystems
    std::shared_ptr<Window> m_mainWindow;
    std::shared_ptr<Physics2D> m_physics2d;
    std::shared_ptr<Viewport2D> m_viewport2d;
    std::shared_ptr<AudioManager> m_audio;
    std::shared_ptr<Gui> m_gui;

    // Service registry
    Services m_services;
};

} // namespace Aeon
