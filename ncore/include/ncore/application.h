// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <memory>

#include <ncore/types/typedb.h>
#include <ncore/utils/config.h>

#include "app_context.h"

namespace nc {

class IGameWorld;
class EventBus;
class WindowService;
class RenderService;
class InputService;

namespace cfg {

struct REFLECT NCAPI Log {
    REFLECT int level        = 0;
    REFLECT String file_path = "logs/engine.log";
    REFLECT String overrides;
};

} // namespace cfg

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
