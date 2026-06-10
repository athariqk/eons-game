#include "Engine.h"

#include <ErrorCodes.h>
#include <Logger.h>
#include <SDLGraphicsContext.h>

namespace Aeon {

Engine::Engine(std::string p_app_name) : m_app_name(p_app_name), m_config(m_configFileName) {
    Logger::Init();
    Init();
}

Engine::~Engine() {}

int Engine::Init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG_INFO("SDL initialized");
    } else {
        LOG_ERROR("SDL failed to initialize!");
        return ERROR_FATAL;
    }

    m_windowConf = m_config.Load<Config::Window>();
    m_renderConf = m_config.Load<Config::Render>();

    m_mainWindow = std::make_shared<Window>(m_app_name.c_str(), m_windowConf.SizeWidth, m_windowConf.SizeHeight,
                                            m_windowConf.Fullscreen);
    m_viewport2d = std::make_shared<Viewport2D>(*m_mainWindow, m_renderConf.PixelsPerMeter);
    m_physics2d = std::make_shared<Physics2D>();
    m_audio = std::make_shared<AudioManager>();
    m_gui = std::make_shared<Gui>(*m_mainWindow);
    auto renderer = dynamic_cast<SDLGraphicsContext *>(m_viewport2d->GetGraphicsContext());
    m_textureManager = std::make_shared<TextureManager>(renderer->GetSDLRenderer());

    const auto resolution = m_mainWindow->GetResolution();
    m_viewport2d->SetRect(0.0f, 0.0f, resolution.x, resolution.y);

    // Register services
    m_services.Register<Window>(*m_mainWindow);
    m_services.Register<Physics2D>(*m_physics2d);
    m_services.Register<Viewport2D>(*m_viewport2d);
    m_services.Register<AudioManager>(*m_audio);
    m_services.Register<Gui>(*m_gui);
    m_services.Register<TextureManager>(*m_textureManager);

    m_mainLoop = std::make_unique<MainLoop>(m_app_name, m_services);
    m_gui->InitSubscriptions(m_mainLoop->GetEventBus());

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
