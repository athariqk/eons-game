// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <memory>
#include <string>

#include <ncore/kernel/types.h>

#include "modules/module_registry.h"

namespace nc {

class IGameWorld;
class EventBus;
class ConfFile;
class IImGuiModule;
class IWindowModule;
class GraphicsModule;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

/**
 * @brief A semantic version representation for the application.
 */
struct NCORE_API AppVersion {
    int Major = 0;
    int Minor = 0;
    int Patch = 0;
    std::string Identifier;
    NSTRUCT(
        AppVersion,
        NC_F( AppVersion, Major ) NC_F( AppVersion, Minor ) NC_F( AppVersion, Patch ) NC_F( AppVersion, Identifier )
    )
};

/**
 * @brief AppDesc can be used to initialize an app with the given specification.
 */
struct NCORE_API AppDesc {
    std::string Name;
    AppVersion Version;
    std::string ConfigFile;
    NSTRUCT( AppDesc, NC_F( AppDesc, Name ) NC_F( AppDesc, Version ) NC_F( AppDesc, ConfigFile ) )
};

#pragma GCC diagnostic pop

/**
 * @brief The entry point for applications.
 *
 * This class handles initialization, driving the game world,
 * OS event polling, and cleanup. You may override this to
 * implement custom main loop behavior.
 */
class NCORE_API Application {
public:
    Application( const AppDesc& desc );
    virtual ~Application();

    Application( const Application& )            = delete;
    Application& operator=( const Application& ) = delete;

    virtual void init();
    virtual void run();
    virtual void finish();

    virtual void poll_events();

    /**
     * @brief Registers the IModules used by the application.
     * This can be overridden to register custom modules.
     */
    virtual void register_modules( ConfFile& cfg_file );

    /**
     * @brief Called once when the application is being destroyed.
     */
    virtual void unregister_modules();

    /**
     * @brief Creates a new game world instance.
     * By default, this creates a new Scene with the default ECS
     * runtime features.
     *
     * See: EcsRuntimeFeature
     */
    virtual std::unique_ptr<IGameWorld> create_world();

    /**
     * @brief Called once after the world is initialized.
     */
    virtual void on_world_init( IGameWorld& world ) {}

protected:
    AppDesc app_desc;
    ModuleRegistry modules;
    bool is_running   = false;
    uint64_t ticks    = 0;
    double delta_time = 0.0;
    std::unique_ptr<IGameWorld> g_world;

    EventBus* events       = nullptr;
    IWindowModule* windows = nullptr;
    GraphicsModule* gfx    = nullptr;
    IImGuiModule* imgui    = nullptr;
};

} // namespace nc
