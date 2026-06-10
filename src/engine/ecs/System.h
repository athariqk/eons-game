#pragma once

#include <cstdint>
#include <memory>

#include <IGraphicsContext.h>

namespace ncore {

class World;

/**
 * @brief Base class for all ECS systems
 *
 * Systems operate on components and have access to engine subsystems.
 * They implement the logic that processes entities with specific component combinations.
 *
 * Systems are updated in priority order (lower priority values execute first).
 */
class System {
public:
    System() = default;
    virtual ~System() = default;

    // Prevent copying
    System(const System &) = delete;
    System &operator=(const System &) = delete;

    /**
     * @brief Called once when the system is added to a world
     * @param world Reference to the world that owns this system
     */
    virtual bool on_init(World &world) { return true; }

    /**
     * @brief Called at a fixed timestep for deterministic updates
     * @param world Reference to the world
     * @param fixedDelta Fixed time delta (e.g., 1/60 seconds)
     *
     * Use this for physics, gameplay logic, and anything that needs to be deterministic.
     */
    virtual void on_fixed_update(World &world, double fixedDelta) {}

    /**
     * @brief Called at variable timestep for non-deterministic updates
     * @param world Reference to the world
     * @param delta Variable time delta
     *
     * Use this for animation, particles, camera smoothing, and other presentation logic.
     */
    virtual void on_variable_update(World &world, double delta) {}

    /**
     * @brief Called after update
     */
    virtual void on_post_update(World &world, double delta) {}

    /**
     * @brief Called during the render phase
     * @param world Reference to the world
     *
     * Use this for rendering entities, debug visualization, etc.
     */
    virtual void on_render(World &world, IGraphicsContext &graphics) {}

    /**
     * @brief Called during the GUI render phase.
	 * Immediate-mode GUI calls can be placed here.
     */
    virtual void on_gui_render(World &world) {}

    /**
     * @brief Called when the world is being destroyed
     * @param world Reference to the world
     */
    virtual void on_shutdown(World &world) {}

    bool get_is_enabled() const { return is_enabled; }
    void set_is_enabled(bool p_is_enabled) { is_enabled = p_is_enabled; }

    // Priority determines update order (lower = earlier)
    int get_priority() const { return priority; }
    void set_priority(int p_priority) { p_priority = p_priority; }

protected:
    bool is_enabled = true;
    int priority = 0;
};

} // namespace ncore
