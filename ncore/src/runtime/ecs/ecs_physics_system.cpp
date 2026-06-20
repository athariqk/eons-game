#include <runtime/ecs/ecs_physics_system.h>

#include <imgui.h>

#include <ncore/runtime/ecs/ecs_rigidbody.h>
#include <ncore/runtime/ecs/ecs_transform.h>
#include <ncore/runtime/ecs_world.h>
#include <ncore/runtime/service_locator.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

namespace ncore {

void EcsPhysicsSystem::on_init(EcsWorld &world) { physics = world.get_services().resolve<IPhysicsService>(); }

void EcsPhysicsSystem::on_fixed_update(EcsWorld &world, double fixedDelta) {
    // for (auto &entity: world.get_entities()) {
    //     if (!world.has<EcsRigidbody>(entity) || !world.has<EcsTransform>(entity))
    //         continue;

    //    auto &body = world.get<EcsRigidbody>(entity);
    //    auto &transform = world.get<EcsTransform>(entity);

    //    if (!b2Body_IsValid(body.b2Id)) {
    //        initialize_rbody(body, transform);
    //    }

    //    if (!body.pending_impulse.is_zero()) {
    //        physics->apply_linear_impulse(body.b2Id, body.pending_impulse);
    //        NC_LOG_DEBUG_C(log::PHYSICS, "Applied impulse <{},{}> to entity {}", body.pending_impulse.x,
    //                       body.pending_impulse.y, entity.id);
    //        body.pending_impulse = {0.0f, 0.0f};
    //    }

    //    if (!body.pending_force.is_zero()) {
    //        physics->apply_linear_force(body.b2Id, body.pending_force);
    //        body.pending_force = {0.0f, 0.0f};
    //    }
    //}

    physics->set_time_step(static_cast<float>(fixedDelta));
    physics->step();

    // for (auto &entity: world.get_entities()) {
    //     if (!world.has<EcsRigidbody>(entity) || !world.has<EcsTransform>(entity))
    //         continue;

    //    auto &body = world.get<EcsRigidbody>(entity);
    //    if (!physics->is_body_awake(body.b2Id))
    //        continue;

    //    auto &transform = world.get<EcsTransform>(entity);

    //    auto b2vel = physics->get_body_velocity(body.b2Id);
    //    body.velocity = Vec2(b2vel.x, b2vel.y);

    //    auto [x, y] = physics->get_body_position(body.b2Id);
    //    transform.position.x = x;
    //    transform.position.y = y;
    //    transform.angle = physics->get_body_angle(body.b2Id);
    //}
}

void EcsPhysicsSystem::on_post_update(EcsWorld &world, double delta) {
    // for (auto &e: world.get_entities()) {
    //     if (!e.is_enabled && world.has<EcsRigidbody>(e)) {
    //         auto &body = world.get<EcsRigidbody>(e);
    //         if (physics->is_body_valid(body.b2Id))
    //             physics->destroy_body(body.b2Id);
    //     }
    // }
}

void EcsPhysicsSystem::on_gui_render(EcsWorld &world) {
    ImGui::Begin("Physics");
    ImGui::Checkbox("Debug Draw", &physics->is_debug_draw);
    ImGui::End();
}

void EcsPhysicsSystem::on_shutdown(EcsWorld &world) { physics->cleanup(); }

void EcsPhysicsSystem::initialize_rbody(EcsRigidbody &body, EcsTransform &transform) {
    // b2BodyDef bodyDef = b2DefaultBodyDef();
    // bodyDef.type = b2_dynamicBody;
    // bodyDef.position = b2Vec2(transform.position.x, transform.position.y);
    // bodyDef.linearDamping = body.linear_damping > 0.0f ? body.linear_damping : 2.5f;
    // body.b2Id = physics->create_body(&bodyDef);

    // Vec2 halfDim = (transform.dimension * transform.scale) * 0.5f;
    // b2Polygon dynamicBox = b2MakeBox(halfDim.x, halfDim.y);

    // b2ShapeDef shapeDef = b2DefaultShapeDef();
    // shapeDef.density = 1.0f;
    // shapeDef.friction = 0.3f;

    // b2CreatePolygonShape(body.b2Id, &shapeDef, &dynamicBox);
    // b2Body_ApplyMassFromShapes(body.b2Id);
}

} // namespace ncore
