#pragma once

#include <modules/physics/physics_service.h>
#include <ncore/runtime/ecs/ecs_system.h>

namespace ncore {

struct EcsRigidbody;
struct EcsTransform;

class EcsPhysicsSystem : public EcsSystem {
    NCLASS(EcsPhysicsSystem, EcsSystem)

public:
    EcsPhysicsSystem() { set_priority(-100); }

    void on_init(EcsWorld &world) override;
    void on_fixed_update(EcsWorld &world, double fixedDelta) override;
    void on_post_update(EcsWorld &world, double delta) override;
    void on_gui_render(EcsWorld &world) override;
    void on_shutdown(EcsWorld &world) override;

private:
    void initialize_rbody(EcsRigidbody &body, EcsTransform &transform);
    IPhysicsService *physics = nullptr;
};

} // namespace ncore
