#pragma once

#include <cstdint>
#include <memory>

#include <IGraphicsContext.h>

namespace Aeon {

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
    virtual bool OnInit(World &world) { return true; }

    /**
     * @brief Called at a fixed timestep for deterministic updates
     * @param world Reference to the world
     * @param fixedDelta Fixed time delta (e.g., 1/60 seconds)
     *
     * Use this for physics, gameplay logic, and anything that needs to be deterministic.
     */
    virtual void OnFixedUpdate(World &world, double fixedDelta) {}

    /**
     * @brief Called at variable timestep for non-deterministic updates
     * @param world Reference to the world
     * @param delta Variable time delta
     *
     * Use this for animation, particles, camera smoothing, and other presentation logic.
     */
    virtual void OnVariableUpdate(World &world, double delta) {}

    /**
     * @brief Called after update
     */
    virtual void OnPostUpdate(World &world, double delta) {}

    /**
     * @brief Called during the render phase
     * @param world Reference to the world
     *
     * Use this for rendering entities, debug visualization, etc.
     */
    virtual void OnRender(World &world, IGraphicsContext &graphics) {}

    /**
     * @brief Called during the GUI render phase. ImGui calls can be placed here.
     */
    virtual void OnGuiRender(World &world) {}

    /**
     * @brief Called when the world is being destroyed
     * @param world Reference to the world
     */
    virtual void OnShutdown(World &world) {}

    // System can be enabled/disabled
    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    // Priority determines update order (lower = earlier)
    int GetPriority() const { return m_priority; }
    void SetPriority(int priority) { m_priority = priority; }

protected:
    bool m_enabled = true;
    int m_priority = 0;
};

} // namespace Aeon
