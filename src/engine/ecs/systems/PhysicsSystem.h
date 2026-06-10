#pragma once

#include <Physics.h>
#include <System.h>

namespace ncore {

struct RigidBodyComponent;
struct TransformComponent;

/**
 * @brief Physics system that manages Box2D physics simulation
 *
 * This system owns the Physics2D instance and is responsible for:
 * - Stepping the physics simulation at a fixed timestep
 * - Syncing physics state to/from TransformComponents
 * - Providing access to physics world for body creation
 *
 * Priority: -100 (runs early, before gameplay logic)
 */
class PhysicsSystem : public System {
public:
    PhysicsSystem() {
        set_priority(-100); // Run physics early
    }

    bool on_init(World &world) override;
    void on_fixed_update(World &world, double fixedDelta) override;
    void on_post_update(World &world, double delta) override;
    void on_gui_render(World &world) override;
    void on_shutdown(World &world) override;

private:
    void initialize_rbody(RigidBodyComponent &body, TransformComponent &transform);

    Physics2D *physics = nullptr;
};

} // namespace ncore
