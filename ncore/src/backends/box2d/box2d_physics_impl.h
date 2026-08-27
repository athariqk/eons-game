#pragma once
#include <unordered_map>

#include <box2d/id.h>
#include <box2d/types.h>

#include <ncore/core/vector.h>
#include <ncore/services/physics/physics_service.h>

namespace nc {

class Box2DPhysicsImpl : public IPhysicsService {
    NCLASS( Box2DPhysicsImpl, IPhysicsService )

public:
    Error init( ConfFile& cfg_file ) override;
    void step() const override;
    void shutdown() override;

    RID create_shape( ShapeType type ) override;

    RID create_rigidbody() override;
    RID create_softbody() override;
    void destroy_body( RID body ) override;

    bool is_body_valid( RID body ) const override;
    bool is_body_awake( RID body ) const override;
    Vec2f get_body_position( RID body ) const override;
    float get_body_angle( RID body ) const override;
    Vec2f get_body_velocity( RID body ) const override;

    void apply_linear_impulse( RID body, const Vec2f& impulse ) override;
    void apply_linear_force( RID body, const Vec2f& force ) override;

    void update_debug_draw() override;
    PhysicsDebugDraw& get_debug_draw() override;

private:
    b2WorldId world_id{};
    std::unordered_map<RID, b2BodyId> body_map;
    b2DebugDraw b2_debug_draw{};
    PhysicsDebugDraw debug_draw;

    void sync_debug_draw_flags();
};

} // namespace nc
