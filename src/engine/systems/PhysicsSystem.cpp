#include "PhysicsSystem.h"

#include <Logger.h>
#include <RigidBodyComponent.h>
#include <TransformComponent.h>
#include <World.h>
#include <imgui.h>

namespace Aeon {

bool PhysicsSystem::OnInit(World &world) {
    m_physics = world.GetMainLoop().GetServices().TryGet<Physics2D>();
    if (!m_physics) {
        LOG_ERROR(Log::Physics, "Physics2D service was not found!");
        return false;
    }

    m_physics->Init();

    return true;
}

void PhysicsSystem::OnFixedUpdate(World &world, double fixedDelta) {
    for (auto &entity: world.GetEntities()) {
        if (!entity->HasComponent<RigidBodyComponent>() || !entity->HasComponent<TransformComponent>())
            continue;

        auto &body = entity->GetComponent<RigidBodyComponent>();
        auto &transform = entity->GetComponent<TransformComponent>();

        if (!b2Body_IsValid(body.b2Id)) {
            InitializeRigidBody(body, transform);
        }

        // Apply pending impulse
        if (!body.pendingImpulse.IsZero()) {
            m_physics->ApplyLinearImpulse(body.b2Id, body.pendingImpulse);
            LOG_TRACE(Log::Physics, "Applied impulse <{},{}> to entity {}", body.pendingImpulse.x, body.pendingImpulse.y,
                      entity->GetID());
            body.pendingImpulse = {0.0f, 0.0f};
        }

        // Apply pending force
        if (!body.pendingForce.IsZero()) {
            m_physics->ApplyLinearForce(body.b2Id, body.pendingForce);
            body.pendingForce = {0.0f, 0.0f};
        }
    }

    m_physics->SetTimeStep(static_cast<float>(fixedDelta));
    m_physics->Step();

    // Sync position back to transform
    for (auto &entity: world.GetEntities()) {
        if (!entity->HasComponent<RigidBodyComponent>() || !entity->HasComponent<TransformComponent>())
            continue;

        auto &body = entity->GetComponent<RigidBodyComponent>();
        if (!b2Body_IsAwake(body.b2Id)) {
            continue; // Skip sleeping bodies
        }

        auto &transform = entity->GetComponent<TransformComponent>();

        auto b2vel = b2Body_GetLinearVelocity(body.b2Id);
        body.velocity = Vector2D(b2vel.x, b2vel.y);

        auto [x, y] = b2Body_GetPosition(body.b2Id);
        transform.position.x = x;
        transform.position.y = y;
        transform.angle = b2Rot_GetAngle(b2Body_GetRotation(body.b2Id));
    }
}

void PhysicsSystem::OnPostUpdate(World &world, double delta) {
    for (auto &e: world.GetEntities()) {
        if (!e->IsEnabled() && e->HasComponent<RigidBodyComponent>()) {
            auto &body = e->GetComponent<RigidBodyComponent>();
            if (m_physics->IsBodyValid(body.b2Id))
                m_physics->DestroyBody(body.b2Id);
        }
    }
}

void PhysicsSystem::OnGuiRender(World &world) {
    ImGui::Begin("Physics");
    ImGui::Checkbox("Debug Draw", &m_physics->isDebugDraw);
    ImGui::End();
}

void PhysicsSystem::OnShutdown(World &world) { m_physics->Clean(); }

void PhysicsSystem::InitializeRigidBody(RigidBodyComponent &body, TransformComponent &transform) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = b2Vec2(transform.position.x, transform.position.y);
    bodyDef.linearDamping = body.linearDamping > 0.0f ? body.linearDamping : 2.5f;
    body.b2Id = m_physics->CreateBody(&bodyDef);

    // b2MakeBox expects half-extents (half-width, half-height)
    // transform.dimension is already the full size, so divide by 2
    Vector2D halfDim = (transform.dimension * transform.scale) * 0.5f;
    b2Polygon dynamicBox = b2MakeBox(halfDim.x, halfDim.y);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 0.3f;

    b2CreatePolygonShape(body.b2Id, &shapeDef, &dynamicBox);
    b2Body_ApplyMassFromShapes(body.b2Id);

    LOG_DEBUG(Log::Physics, "ID: {}, mass={}, type={}, bodyPos=<{},{}>, transform={}", body.entity->GetID(),
              b2Body_GetMass(body.b2Id),
              b2Body_GetType(body.b2Id) == b2_staticBody
                  ? "static"
                  : (b2Body_GetType(body.b2Id) == b2_kinematicBody ? "kinematic" : "dynamic"),
              bodyDef.position.x, bodyDef.position.y, transform.ToString());
}

} // namespace Aeon

