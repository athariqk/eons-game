#pragma once

#include <Physics.h>
#include <System.h>

namespace Aeon {

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
        SetPriority(-100); // Run physics early
    }

    bool OnInit(World &world) override;
    void OnFixedUpdate(World &world, double fixedDelta) override;
    void OnGuiRender(World &world) override;

private:
    void InitializeRigidBody(RigidBodyComponent &body, TransformComponent &transform);

    Physics2D *m_physics = nullptr;
};

} // namespace Aeon
