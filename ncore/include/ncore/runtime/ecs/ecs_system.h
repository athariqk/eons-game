#pragma once

#include <ncore/kernel/object.h>

namespace ncore {

class EcsWorld;
class IRenderService;
class ServiceLocator;

/**
 * @brief Base class for all ECS systems
 *
 * Systems are updated in priority order (lower priority values execute first).
 */
class EcsSystem : public NObject {
    NCLASS(EcsSystem, NObject)

public:
    virtual ~EcsSystem() = default;

    /**
     * @brief Called once when the system is added to a world
     * @param world Reference to the world that owns this system
     */
    virtual void on_init(EcsWorld &world) {}

    /**
     * @brief Called at a fixed timestep for deterministic updates
     * @param world Reference to the world
     * @param fixedDelta Fixed time delta (e.g., 1/60 seconds)
     *
     * Use this for physics, gameplay logic, and anything that needs to be deterministic.
     */
    virtual void on_fixed_update(EcsWorld &world, double fixedDelta) {}

    /**
     * @brief Called at variable timestep for non-deterministic updates
     * @param world Reference to the world
     * @param delta Variable time delta
     *
     * Use this for animation, particles, camera smoothing, and other presentation logic.
     */
    virtual void on_variable_update(EcsWorld &world, double delta) {}

    /**
     * @brief Called after update
     * @param world Reference to the world
     * @param delta Time delta
     */
    virtual void on_post_update(EcsWorld &world, double delta) {}

    /**
     * @brief Called during the render phase
     * @param world Reference to the world
     *
     * Use this for rendering entities, debug visualization, etc.
     */
    virtual void on_render(EcsWorld &world, IRenderService &graphics) {}

    /**
     * @brief Called during the GUI render phase.
     * Immediate-mode GUI calls can be placed here.
     */
    virtual void on_gui_render(EcsWorld &world) {}

    /**
     * @brief Called when the world is being destroyed
     * @param world Reference to the world
     */
    virtual void on_shutdown(EcsWorld &world) {}

    bool get_is_enabled() const { return is_enabled; }
    void set_is_enabled(bool p_is_enabled) { is_enabled = p_is_enabled; }

    // Priority determines update order (lower = earlier)
    int get_priority() const { return priority; }
    void set_priority(int p_priority) { priority = p_priority; }

    // Context access declarations (for future parallel scheduler)
    // Override to declare which context types this system reads/writes.
    // Empty = assumed to access everything (runs in sequential mode).
    virtual std::vector<size_t> read_context_ids() const { return {}; }
    virtual std::vector<size_t> write_context_ids() const { return {}; }

protected:
    bool is_enabled = true;
    int priority = 0;
};

} // namespace ncore
