#pragma once

#include <memory>
#include <string>

#include <ncore/kernel/types.h>
#include <ncore/modules/events/event_bus.h>

namespace ncore {

class IGameWorld;
class ServiceLocator;

/**
 * @brief A semantic version representation for the application.
 */
struct AppVersion {
    int Major = 0;
    int Minor = 0;
    int Patch = 0;
    std::string Identifier;
    NSTRUCT(AppVersion,
            NC_F(AppVersion, Major) NC_F(AppVersion, Minor) NC_F(AppVersion, Patch) NC_F(AppVersion, Identifier));
};

/**
 * @brief The entry point for applications.
 *
 * This class handles initialization, driving the game world,
 * OS event polling, and cleanup.
 */
class Application {
public:
    Application(const std::string &name, AppVersion version, const std::string &config_file);
    virtual ~Application();

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    virtual void init();
    virtual void run();
    virtual void finish();

    virtual void poll_events();

    bool get_is_running() const { return is_running; }

    virtual std::unique_ptr<IGameWorld> create_world() = 0;

    EventBus &get_event_bus() { return event_bus; }

protected:
    std::string app_name;
    AppVersion app_version;
    bool is_running = false;
    EventBus event_bus;
    ServiceLocator &services;
    std::string config_file;
    uint64_t ticks = 0;
    double delta_time = 0.0;
    std::unique_ptr<IGameWorld> g_world;
};

} // namespace ncore
