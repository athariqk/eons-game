// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <memory>
#include <string>

#include <ncore/core/types.h>

#include "services/service_registry.h"

namespace nc {

class IGameWorld;
class ResourceService;
class EventBus;
class ConfFile;
class WindowService;
class RenderService;
class InputService;

/**
 * @brief A semantic version representation for the application.
 */
struct NCAPI AppVersion {
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
struct NCAPI AppDesc {
    std::string Name;
    AppVersion Version;
    std::string ConfigFile;
    NSTRUCT( AppDesc, NC_F( AppDesc, Name ) NC_F( AppDesc, Version ) NC_F( AppDesc, ConfigFile ) )
};

/**
 * @brief The entry point for applications.
 *
 * This class handles initialization, driving the game world,
 * OS event polling, and cleanup. You may override this to
 * implement custom main loop behavior.
 */
class NCAPI Application {
public:
    Application( const AppDesc& desc );
    virtual ~Application();

    Application( const Application& )            = delete;
    Application& operator=( const Application& ) = delete;

    /**
     * @brief Sets up the application, initializing important subsystems.
     */
    virtual void init();
    /**
     * @brief Runs the game loop until the end.
     */
    virtual void run();
    /**
     * @brief Performs teardown, cleanup, and shutting down subsystems.
     */
    virtual void finish();

    virtual void process_events();

    /**
     * @brief Registers the IServices used by the application.
     * This can be overridden to register custom services.
     */
    virtual void register_services();

    /**
     * @brief Called once when the application is being destroyed.
     */
    virtual void unregister_services();

    /**
     * @brief Creates a new game world instance.
     * By default, this creates a new Scene with the default ECS
     * runtime features.
     *
     * See: EcsBaseFeatures
     */
    virtual std::unique_ptr<IGameWorld> create_world();

protected:
    AppDesc app_desc;
    ServiceRegistry services;
    bool is_running   = false;
    uint64_t ticks    = 0;
    double delta_time = 0.0;
    std::unique_ptr<IGameWorld> g_world;

    ResourceService* resources = nullptr;
    EventBus* events           = nullptr;
    WindowService* window      = nullptr;
    RenderService* renderer    = nullptr;
    InputService* input        = nullptr;
};

} // namespace nc
