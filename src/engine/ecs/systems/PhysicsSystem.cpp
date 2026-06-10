#include "PhysicsSystem.h"

#include <Logger.h>
#include <RigidBodyComponent.h>
#include <TransformComponent.h>
#include <World.h>
#include <imgui.h>

namespace ncore {

bool PhysicsSystem::on_init(World &world) {
    physics = world.get_main_loop().get_services().try_get<Physics2D>();
    if (!physics) {
        LOG_ERROR(log::PHYSICS, "Physics2D service was not found!");
        return false;
    }

    physics->init();

    return true;
}

void PhysicsSystem::on_fixed_update(World &world, double fixedDelta) {
    for (auto &entity: world.get_entities()) {
        if (!entity->has_component<RigidBodyComponent>() || !entity->has_component<TransformComponent>())
            continue;

        auto &body = entity->get_component<RigidBodyComponent>();
        auto &transform = entity->get_component<TransformComponent>();

        if (!b2Body_IsValid(body.b2Id)) {
            initialize_rbody(body, transform);
        }

        // Apply pending impulse
        if (!body.pending_impulse.is_zero()) {
            physics->apply_linear_impulse(body.b2Id, body.pending_impulse);
            LOG_TRACE(log::PHYSICS, "Applied impulse <{},{}> to entity {}", body.pending_impulse.x,
                      body.pending_impulse.y, entity->get_id());
            body.pending_impulse = {0.0f, 0.0f};
        }

        // Apply pending force
        if (!body.pending_force.is_zero()) {
            physics->apply_linear_force(body.b2Id, body.pending_force);
            body.pending_force = {0.0f, 0.0f};
        }
    }

    physics->set_time_step(static_cast<float>(fixedDelta));
    physics->step();

    // Sync position back to transform
    for (auto &entity: world.get_entities()) {
        if (!entity->has_component<RigidBodyComponent>() || !entity->has_component<TransformComponent>())
            continue;

        auto &body = entity->get_component<RigidBodyComponent>();
        if (!physics->is_body_awake(body.b2Id)) {
            continue; // Skip sleeping bodies
        }

        auto &transform = entity->get_component<TransformComponent>();

        auto b2vel = physics->get_body_velocity(body.b2Id);
        body.velocity = Vec2D(b2vel.x, b2vel.y);

        auto [x, y] = physics->get_body_position(body.b2Id);
        transform.position.x = x;
        transform.position.y = y;
        transform.angle = physics->get_body_angle(body.b2Id);
    }
}

void PhysicsSystem::on_post_update(World &world, double delta) {
    for (auto &e: world.get_entities()) {
        if (!e->is_enabled() && e->has_component<RigidBodyComponent>()) {
            auto &body = e->get_component<RigidBodyComponent>();
            if (physics->is_body_valid(body.b2Id))
                physics->destroy_body(body.b2Id);
        }
    }
}

void PhysicsSystem::on_gui_render(World &world) {
    ImGui::Begin("Physics");
    ImGui::Checkbox("Debug Draw", &physics->is_debug_draw);
    ImGui::End();
}

void PhysicsSystem::on_shutdown(World &world) { physics->clean(); }

void PhysicsSystem::initialize_rbody(RigidBodyComponent &body, TransformComponent &transform) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = b2Vec2(transform.position.x, transform.position.y);
    bodyDef.linearDamping = body.linear_damping > 0.0f ? body.linear_damping : 2.5f;
    body.b2Id = physics->create_body(&bodyDef);

    // b2MakeBox expects half-extents (half-width, half-height)
    // transform.dimension is already the full size, so divide by 2
    Vec2D halfDim = (transform.dimension * transform.scale) * 0.5f;
    b2Polygon dynamicBox = b2MakeBox(halfDim.x, halfDim.y);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 0.3f;

    b2CreatePolygonShape(body.b2Id, &shapeDef, &dynamicBox);
    b2Body_ApplyMassFromShapes(body.b2Id);

    LOG_DEBUG(log::PHYSICS, "ID: {}, mass={}, type={}, bodyPos=<{},{}>, transform={}", body.entity->get_id(),
              b2Body_GetMass(body.b2Id),
              b2Body_GetType(body.b2Id) == b2_staticBody
                  ? "static"
                  : (b2Body_GetType(body.b2Id) == b2_kinematicBody ? "kinematic" : "dynamic"),
              bodyDef.position.x, bodyDef.position.y, transform.to_string());
}

} // namespace ncore
