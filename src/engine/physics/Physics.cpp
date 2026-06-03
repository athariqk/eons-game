#include "Physics.h"

#include <Logger.h>
#include <Vector2D.h>

namespace Aeon {

Physics2D::Physics2D() {
    // Create physics world with no gravity
    m_worldDef = b2DefaultWorldDef();
    m_worldDef.gravity.y = 0;
    m_worldId = b2CreateWorld(&m_worldDef);
}

void Physics2D::Step() const {
    if (!b2World_IsValid(m_worldId)) {
        LOG_ERROR("Trying to step simulation without physics world!");
        return;
    }

    b2World_Step(m_worldId, GetTimeStep(), GetSubStepCount());
}

b2BodyId Physics2D::CreateBody(const b2BodyDef *bodyDef) const {
    if (!b2World_IsValid(m_worldId))
        return {0};

    return b2CreateBody(m_worldId, bodyDef);
}

void Physics2D::DestroyBody(const b2BodyId &bodyId) const {
    if (!b2World_IsValid(m_worldId)) {
        LOG_ERROR("Trying to destroy body without physics world!");
        return;
    }
    if (!IsBodyValid(bodyId)) {
        LOG_ERROR("Trying to destroy an invalid body!");
        return;
    }
    b2DestroyBody(bodyId);
}

bool Physics2D::IsBodyValid(const b2BodyId &bodyId) const { return b2Body_IsValid(bodyId); }

void Physics2D::ApplyLinearImpulse(const b2BodyId &bodyId, const Vector2D &impulse) const {
    if (!IsBodyValid(bodyId)) {
        LOG_ERROR("Can't apply impulse on a uninitialized body");
        return;
    }

    b2Body_ApplyLinearImpulseToCenter(bodyId, b2Vec2(impulse.x, impulse.y), true);
}

void Physics2D::ApplyLinearForce(const b2BodyId &bodyId, const Vector2D &force) const {
    if (!IsBodyValid(bodyId)) {
        LOG_ERROR("Can't apply force on a uninitialized body");
        return;
    }

    b2Body_ApplyForceToCenter(bodyId, b2Vec2(force.x, force.y), true);
}

} // namespace Aeon
