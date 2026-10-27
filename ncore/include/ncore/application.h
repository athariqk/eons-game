// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <memory>

#include <ncore/core/types.h>
#include <ncore/utils/config.h>

#include "services/service_registry.h"

namespace nc {

class IGameWorld;
class EventBus;
class WindowService;
class RenderService;
class InputService;

/**
 * @brief A semantic version representation for the application.
 */
struct REFLECT NCAPI AppVersion {
    REFLECT int major         = 0;
    REFLECT int minor         = 0;
    REFLECT int patch         = 0;
    REFLECT String identifier = "n";
};

/**
 * @brief AppDesc can be used to initialize an app with the given specification.
 */
struct REFLECT NCAPI AppDesc {
    REFLECT String name;        // Name of the application.
    REFLECT AppVersion version; // Semantic version of the application.
    REFLECT String config_file; // Relative path to the application's config file.
};

namespace cfg {

struct REFLECT NCAPI Log {
    REFLECT int level        = 0;
    REFLECT String file_path = "logs/engine.log";
    REFLECT String overrides;
};

} // namespace cfg

/**
 * @brief State of an Application object.
 */
struct NCAPI AppContext {
    AppDesc app_desc;          // Descriptor used to init this application.
    ConfFile config;           // Parsed application config (loaded once in Application::init).
    ServiceRegistry services;  // Registry of all registered engine services.
    bool is_running   = false; // Whether game loop is still looping.
    uint64_t ticks    = 0;     // Ticks since first run() frame.
    double delta_time = 0.0;
};

/**
 * @brief The entry point for game applications.
 *
 * This class handles initialization, driving the game world,
 * OS event polling, and cleanup.
 *
 * You may override this to implement custom app behavior.
 */
class NCAPI Application {
public:
    Application( const AppDesc& p_desc );
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
     * @brief Creates initial game world instance.
     * By default, this creates a new Scene.
     */
    virtual Ptr<IGameWorld> create_world();

protected:
    AppContext context;         // Current application state.
    Ptr<IGameWorld> game_world; // Current active game world.

    EventBus* events        = nullptr;
    WindowService* window   = nullptr;
    RenderService* renderer = nullptr;
    InputService* input     = nullptr;
};

} // namespace nc
