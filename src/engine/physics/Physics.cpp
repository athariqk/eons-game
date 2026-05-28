#include "Physics.h"

#include "Logger.h"

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
