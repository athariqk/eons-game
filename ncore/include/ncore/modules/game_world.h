#pragma once

#include <ncore/kernel/object.h>

namespace ncore {

class ServiceLocator;

/**
 * @brief The game world for an application.
 *
 * Derived classes should implement the specific behavior of the game world.
 */
class IGameWorld : public NObject {
    NCLASS(IGameWorld, NObject)

public:
    IGameWorld(ServiceLocator& services) : services(services) {}
    virtual ~IGameWorld() = default;

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
    virtual bool on_fixed_update(double p_delta) = 0;

    /**
     * @brief Called at a variable timestep, ideal for non-deterministic updates.
     *
     * Return false to continue the main loop.
     */
    virtual bool on_variable_update(double p_delta) = 0;

    /**
     * @brief Called once when the world is being destroyed, ideal for cleanup.
     */
    virtual void on_finish() = 0;

    ServiceLocator& get_services() const
    {
        return services;
    }

protected:
    ServiceLocator& services;
};

} // namespace ncore
