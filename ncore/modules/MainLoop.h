#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <modules/Services.h>
#include <modules/World.h>
#include <modules/events/EventBus.h>
#include <modules/events/InputEvent.h>

enum SDL_Scancode;

namespace ncore {

struct WorldChangeRequestEvent : public Event {
    NC_REG_EVENT_TYPE(EventType::WORLD_CHANGE_REQUEST)
    std::unique_ptr<World> new_world;
    WorldChangeRequestEvent(std::unique_ptr<World> p_new_world) : new_world(std::move(p_new_world)) {}
};

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

    /**
     * @brief Request a world change.
     * World change happens at the start of the next frame.
     */
    void change_world(std::unique_ptr<World> p_world);
    World &get_current_world();

    uint64_t &get_tick() { return ticks; }

    // Access to the engine's main services
    Services &get_services() { return service_reg; }

    EventBus &get_event_bus() { return event_bus; }

private:
    void update_active_world(std::unique_ptr<World> p_new_world);

private:
    bool is_running = false;
    std::string app_name;
    Services &service_reg;
    EventBus event_bus;
    std::unique_ptr<World> active_world;
    uint64_t ticks = 0;
};

} // namespace ncore
