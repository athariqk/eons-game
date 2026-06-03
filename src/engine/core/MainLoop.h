#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <InputEvents.h>
#include <Services.h>

enum SDL_Scancode;

namespace Aeon {

class World;

class MainLoop {
public:
    MainLoop(const std::string &p_appName, Services &p_serviceRegistry);
    ~MainLoop();

    int Run();
    void Stop() { m_running = false; }
    bool IsRunning() const { return m_running; }

    /**
     * @brief Poll events from the OS side
     */
    void PollEvents();

    void FixedUpdate(double p_delta);

    void Update(double p_delta);

    void Clean();

    void ChangeWorld(std::unique_ptr<World> p_world);
    World &GetCurrentWorld();

    uint64_t &GetTick() { return m_ticks; }

	// Access to the engine's main services
    Services &GetServices() { return m_services; }

	EventBus &GetEventBus() { return m_eventBus; }

private:
    void UpdateActiveWorld();

private:
    bool m_running = false;

    std::string m_appName;

    // Referenced service registry
    Services &m_services;

    // Event bus
    EventBus m_eventBus;

    std::unique_ptr<World> m_activeWorld;
    std::unique_ptr<World> m_nextWorld;
    bool m_worldDirty = false;

    uint64_t m_ticks = 0;

    KeyboardEvent::Key MapSDLKeyToKey(SDL_Scancode scancode);
};

} // namespace Aeon
