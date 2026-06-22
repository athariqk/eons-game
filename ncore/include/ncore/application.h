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
 * @brief AppDesc can be used to initialize an app with the given specification.
 */
struct AppDesc {
    std::string Name;
    AppVersion Version;
    std::string ConfigFile;
    NSTRUCT(AppDesc, NC_F(AppDesc, Name) NC_F(AppDesc, Version) NC_F(AppDesc, ConfigFile));
};

/**
 * @brief The entry point for applications.
 *
 * This class handles initialization, driving the game world,
 * OS event polling, and cleanup. You may override this to
 * implement custom main loop behavior.
 */
class Application {
public:
    Application(const AppDesc &desc);
    virtual ~Application();

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    virtual void init();
    virtual void run();
    virtual void finish();

    virtual void poll_events();
    virtual std::unique_ptr<IGameWorld> create_world() = 0;

protected:
    AppDesc app_desc;
    ServiceLocator &services;
    bool is_running = false;
    uint64_t ticks = 0;
    double delta_time = 0.0;
    std::unique_ptr<IGameWorld> g_world;
    EventBus *event_bus;
};

} // namespace ncore
