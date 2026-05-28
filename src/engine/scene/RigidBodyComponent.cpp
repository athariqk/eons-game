#include "RigidBodyComponent.h"

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <cmath>

#include <Scene.h>
#include "Physics.h"
#include "box2d/box2d.h"

#include "TransformComponent.h"

#include "Logger.h"

void RigidBodyComponent::OnInit() {
    transform = &entity->GetComponent<TransformComponent>();

    Scene *scene = entity->GetManager().GetScene();
    if (!scene || !scene->GetPhysics2D()) {
        LOG_ERROR("Scene or Physics2D not available in RigidBodyComponent::OnInit");
        return;
    }

    // Validate position values before passing to Box2D
    float posX = transform->position.x;
    float posY = transform->position.y;

	LOG_DEBUG("[RigidBodyComponent::OnInit] ID: {}, TransformComponent<{}, {}>", entity->GetID(), posX, posY);

    if (!std::isfinite(posX) || !std::isfinite(posY)) {
        LOG_ERROR("Invalid position ({}, {}) detected in RigidBodyComponent::OnInit, defaulting to (0, 0)", posX, posY);
        posX = 0.0f;
        posY = 0.0f;
        transform->position.x = posX;
        transform->position.y = posY;
    }

    // Validate dimensions
    float halfWidth = transform->width * transform->scale / 2;
    float halfHeight = transform->height * transform->scale / 2;

    if (!std::isfinite(halfWidth) || !std::isfinite(halfHeight) || halfWidth <= 0.0f || halfHeight <= 0.0f) {
        LOG_ERROR("Invalid dimensions (w: {}, h: {}) detected in RigidBodyComponent::OnInit", 
                  transform->width, transform->height);
        return;
    }

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = b2Vec2(posX, posY);
    bodyDef.linearDamping = 0.5f;

    bodyId = scene->GetPhysics2D()->CreateBody(&bodyDef);

    b2Polygon dynamicBox = b2MakeBox(halfWidth, halfHeight);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 0.3f;

    b2CreatePolygonShape(bodyId, &shapeDef, &dynamicBox);
}

void RigidBodyComponent::OnUpdate(float delta) {
    auto [x, y] = b2Body_GetPosition(bodyId);
    transform->position.x = x;
    transform->position.y = y;
}

void RigidBodyComponent::OnDraw() {
    // Debug drawing can be added later when GUI system is properly integrated
}

void RigidBodyComponent::ApplyLinearImpulse(const Vector2D &impulse) const {
    if (!b2Body_IsValid(bodyId)) {
        LOG_ERROR("Can't apply impulse on a uninitialized body");
        return;
    }

    b2Body_ApplyLinearImpulseToCenter(bodyId, b2Vec2(impulse.x, impulse.y), true);
}

void RigidBodyComponent::ApplyLinearForce(const Vector2D &force) const {
    if (!b2Body_IsValid(bodyId)) {
        LOG_ERROR("Can't apply force on a uninitialized body");
        return;
    }

    b2Body_ApplyForceToCenter(bodyId, b2Vec2(force.x, force.y), true);
}

void RigidBodyComponent::OnClear() {}
