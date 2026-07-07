#pragma once

#include <ncore/kernel/object.h>

namespace nc {

class ModuleRegistry;

/**
 * @brief The game world for an application.
 *
 * Derived classes should implement the specific behavior of the game world.
 */
class NCORE_API IGameWorld : public NcObject {
    NCLASS( IGameWorld, NcObject )

public:
    IGameWorld( ModuleRegistry& p_modules ) : modules( p_modules ) {}

    // Lifecycle hooks

    /**
     * @brief Called once when the world is created.
     */
    virtual void on_init() = 0;

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
    virtual void on_finish() = 0;

    ModuleRegistry& get_modules() const
    {
        return modules;
    }

protected:
    ModuleRegistry& modules;
};

} // namespace nc
