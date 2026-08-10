#pragma once

#include <ncore/core/object.h>

namespace nc {

struct AppDesc;
class ServiceRegistry;

/**
 * @brief The game world for an application.
 *
 * Derived classes should implement the specific behavior of the game world.
 */
class NCAPI IGameWorld : public NcObject {
    NCLASS( IGameWorld, NcObject )

public:
    IGameWorld( AppDesc& p_app_desc, ServiceRegistry& p_services ) : app_desc( p_app_desc ), services( p_services ) {}

    // Lifecycle hooks

    /**
     * @brief Called once when the world has become active.
     */
    virtual void on_enter() = 0;

    /**
     * @brief Called after on_enter() and before the first tick.
     */
    virtual void on_ready() = 0;

    /**
     * @brief Called at a fixed timestep, ideal for physics and deterministic updates.
     *
     * Return false to continue the main loop.
     */
    virtual bool on_fixed_update( double p_delta ) = 0;

    /**
     * @brief Called at a variable timestep, ideal for non-deterministic updates.
     *
     * Return false to continue the main loop.
     */
    virtual bool on_variable_update( double p_delta ) = 0;

    /**
     * @brief Called once when the world is being destroyed, ideal for cleanup.
     */
    virtual void on_exit() = 0;

    /**
     * @brief Requests the world to quit at the next update tick.
     */
    void request_quit()
    {
        wants_to_quit = true;
        NC_LOG_DEBUG( "Quit requested" );
    }

    /**
     * @brief Returns true if quit has been requested.
     */
    bool is_quit_requested() const
    {
        return wants_to_quit;
    }

    ServiceRegistry& get_services() const
    {
        return services;
    }

    AppDesc& get_app_desc() const
    {
        return app_desc;
    }

protected:
    AppDesc& app_desc;
    ServiceRegistry& services;
    bool wants_to_quit = false;
};

} // namespace nc
