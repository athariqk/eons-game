#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <InputEvents.h>
#include <Services.h>

enum SDL_Scancode;

namespace ncore {

class World;

class MainLoop {
public:
    MainLoop(const std::string &p_app_name, Services &p_service_reg);
    ~MainLoop();

    int run();

    /**
     * @brief Forcefully stops the main loop
	 *
	 * Typically you would enqueue an event to do this
     */
    void stop() { is_running = false; }

    bool get_is_running() const { return is_running; }

    /**
     * @brief Poll events from the OS side
     */
    void poll_events();

    void fixed_update(double p_delta);

    void update(double p_delta);

    void clean();

    void change_world(std::unique_ptr<World> p_world);
    World &get_current_world();

    uint64_t &get_tick() { return ticks; }

    // Access to the engine's main services
    Services &get_services() { return service_reg; }

    EventBus &get_event_bus() { return event_bus; }

private:
    void update_active_world();

private:
    bool is_running = false;
    std::string app_name;
    Services &service_reg;
    EventBus event_bus;
    std::unique_ptr<World> active_world;
    std::unique_ptr<World> next_world;
    bool is_world_dirty = false;
    uint64_t ticks = 0;
};

} // namespace ncore
