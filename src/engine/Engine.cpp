#include "Engine.h"

#include <ErrorCodes.h>
#include <Logger.h>

namespace Aeon {

Engine::Engine(std::string p_app_name) : m_app_name(p_app_name) {
    Logger::Init();
    LoadConfiguration();
    Init();
}

Engine::~Engine() {}

int Engine::GetWindowSizeWidth() {
    const auto &width = m_config["graphics"]["window_size_width"];
    return std::stoi(width.empty() ? "800" : width);
}

int Engine::GetWindowSizeHeight() {
    const auto &height = m_config["graphics"]["window_size_height"];
    return std::stoi(height.empty() ? "800" : height);
}

bool Engine::GetWindowFullscreen() {
    const auto &fullscreen = m_config["graphics"]["window_fullscreen"];
    return fullscreen == "true" || fullscreen == "1";
}

int Engine::LoadConfiguration() {
    mINI::INIFile configFile(m_configFileName);
    configFile.read(m_config);
    return ERROR_OK;
}

int Engine::Init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG_INFO("SDL initialized");
    } else {
        LOG_ERROR("SDL failed to initialize!");
        return ERROR_FATAL;
    }

    int win_width = GetWindowSizeWidth();
    int win_height = GetWindowSizeHeight();
    bool win_fullscreen = GetWindowFullscreen();

    m_mainWindow = std::make_shared<Window>(m_app_name.c_str(), win_width, win_height, win_fullscreen);
    m_physics2d = std::make_shared<Physics2D>();
    m_viewport2d = std::make_shared<Viewport2D>(*m_mainWindow);
    m_audio = std::make_shared<AudioManager>();
    m_gui = std::make_shared<Gui>(*m_mainWindow);

    const auto resolution = m_mainWindow->GetResolution();
    m_viewport2d->SetRect(0.0f, 0.0f, resolution.x, resolution.y);

    // Register services
    m_services.Register<Window>(*m_mainWindow);
    m_services.Register<Physics2D>(*m_physics2d);
    m_services.Register<Viewport2D>(*m_viewport2d);
    m_services.Register<AudioManager>(*m_audio);
    m_services.Register<Gui>(*m_gui);

    m_mainLoop = std::make_unique<MainLoop>(m_app_name, m_services);

    return ERROR_OK;
}

int Engine::Run() {
    if (!m_mainLoop) {
        LOG_ERROR("MainLoop is not initialized!");
        return ERROR_FATAL;
    }

    int result = m_mainLoop->Run();
    Cleanup();
    return result;
}

int Engine::Cleanup() {
    if (m_audio) {
        m_audio->Clear();
    }

	if (m_gui) {
		m_gui->Clear();
    }

    m_mainLoop.reset();
    m_viewport2d.reset();
    m_mainWindow.reset();
    m_audio.reset();
    m_gui.reset();

    SDL_Quit();

    return ERROR_OK;
}

} // namespace Aeon
