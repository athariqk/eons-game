#pragma once
#include <unordered_map>

#include <box2d/id.h>
#include <box2d/types.h>

#include <modules/physics/physics_service.h>
#include <ncore/kernel/structures.h>

namespace ncore {

class AssetManager;

class Box2DPhysicsImpl : public IPhysicsService {
    NCLASS(Box2DPhysicsImpl, IPhysicsService)

public:
    Box2DPhysicsImpl() = default;

    Error init() override;
    void step() const override;
    void finalize() override;

    RID create_shape(ShapeType type) override;

    RID create_rigidbody() override;
    RID create_softbody() override;
    void destroy_body(RID body) override;

    bool is_body_valid(RID body) const override;
    bool is_body_awake(RID body) const override;
    Vec2 get_body_position(RID body) const override;
    float get_body_angle(RID body) const override;
    Vec2 get_body_velocity(RID body) const override;

    void apply_linear_impulse(RID body, const Vec2& impulse) override;
    void apply_linear_force(RID body, const Vec2& force) override;

    void update_debug_draw() override;
    DebugDrawFnc& get_debug_draw_fnc() override;

private:
    b2WorldId world_id{0};
    std::unordered_map<RID, b2BodyId> body_map;
    b2DebugDraw b2_debug_draw{0};
    DebugDrawFnc debug_draw_fnc;

    void sync_debug_draw_flags();
};

} // namespace ncore
