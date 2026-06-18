#pragma once

#include <ncore/kernel/config.h>
#include <ncore/kernel/errors.h>
#include <ncore/kernel/object.h>
#include <ncore/kernel/types.h>
#include <ncore/kernel/world.h>
#include <ncore/modules/events/event_bus.h>

namespace ncore {

class ServiceLocator;

namespace cfg {

struct Log {
    int Level = 0;
    std::string FilePath = "logs/engine.log";
    std::string Overrides;
    NC_BIND(Log, NC_F(Log, Level) NC_F(Log, FilePath) NC_F(Log, Overrides));
};

} // namespace cfg

/**
 * @brief The main loop interface and entry point for applications.
 *
 * This class provides the basic structure for an application main loop.
 * It handles initialization, event polling, and cleanup. Derived classes
 * should implement the specific behavior of the main loop.
 */
class MainLoop : public NcObject {
    NCLASS(MainLoop, NcObject)

public:
    explicit MainLoop(std::string app_name);
    virtual ~MainLoop();

    Error run(IWorld *world);
    void stop() { is_running = false; }
    bool get_is_initialized() const { return is_initialized; }
    bool get_is_running() const { return is_running; }
    IWorld &get_world();

    ServiceLocator &get_services() { return services; }
    EventBus &get_event_bus() { return event_bus; }

protected:
    virtual Error init(IWorld &world) = 0;
    virtual Error main_loop() = 0;
    virtual Error poll_events() = 0;
    virtual Error cleanup() = 0;

    std::string app_name;
    ConfFile cfg_file;
    ServiceLocator &services;
    EventBus event_bus;

private:
    bool is_initialized = false;
    bool is_running = false;
    IWorld *active_world = nullptr;
};

} // namespace ncore
