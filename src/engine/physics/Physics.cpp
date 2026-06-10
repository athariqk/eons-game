#include "Physics.h"

#include <Logger.h>
#include <Vector2D.h>
#include <cmath>

namespace Aeon {

// ---- Forwarding callbacks: b2DebugDraw -> DebugDrawFnc ----

static void OnB2DrawPolygon(const b2Vec2 *vertices, int vertexCount, b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->DrawPolygon)
        return;
    Vector2D buf[8];
    int n = vertexCount < 8 ? vertexCount : 8;
    for (int i = 0; i < n; i++)
        buf[i] = Vector2D(vertices[i].x, vertices[i].y);
    wrapper->DrawPolygon(buf, n, static_cast<uint32_t>(color), wrapper->context);
}

static void OnB2DrawSolidPolygon(b2Transform transform, const b2Vec2 *vertices, int vertexCount, float radius,
                                 b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->DrawSolidPolygon)
        return;
    Vector2D buf[8];
    int n = vertexCount < 8 ? vertexCount : 8;
    for (int i = 0; i < n; i++) {
        b2Vec2 wp = b2TransformPoint(transform, vertices[i]);
        buf[i] = Vector2D(wp.x, wp.y);
    }
    wrapper->DrawSolidPolygon(buf, n, radius, static_cast<uint32_t>(color), wrapper->context);
}

static void OnB2DrawCircle(b2Vec2 center, float radius, b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->DrawCircle)
        return;
    wrapper->DrawCircle(Vector2D(center.x, center.y), radius, static_cast<uint32_t>(color), wrapper->context);
}

static void OnB2DrawSolidCircle(b2Transform transform, float radius, b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->DrawSolidCircle)
        return;
    wrapper->DrawSolidCircle(Vector2D(transform.p.x, transform.p.y), radius, static_cast<uint32_t>(color),
                             wrapper->context);
}

static void OnB2DrawSolidCapsule(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->DrawSolidCapsule)
        return;
    wrapper->DrawSolidCapsule(Vector2D(p1.x, p1.y), Vector2D(p2.x, p2.y), radius, static_cast<uint32_t>(color),
                              wrapper->context);
}

static void OnB2DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->DrawSegment)
        return;
    wrapper->DrawSegment(Vector2D(p1.x, p1.y), Vector2D(p2.x, p2.y), static_cast<uint32_t>(color), wrapper->context);
}

static void OnB2DrawTransform(b2Transform transform, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->DrawTransform)
        return;
    float angle = b2Rot_GetAngle(transform.q);
    wrapper->DrawTransform(Vector2D(transform.p.x, transform.p.y), angle, wrapper->context);
}

static void OnB2DrawPoint(b2Vec2 p, float size, b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->DrawPoint)
        return;
    wrapper->DrawPoint(Vector2D(p.x, p.y), size, static_cast<uint32_t>(color), wrapper->context);
}

// ---------- IMPL ------

void Physics2D::Init() {
    if (b2World_IsValid(m_worldId)) {
        return; // already initialized
    }

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity.y = 0;
    m_worldId = b2CreateWorld(&worldDef);

    m_b2DebugDraw = b2DefaultDebugDraw();
    m_b2DebugDraw.context = &m_debugDrawFnc;
    m_b2DebugDraw.DrawPolygon = OnB2DrawPolygon;
    m_b2DebugDraw.DrawSolidPolygon = OnB2DrawSolidPolygon;
    m_b2DebugDraw.DrawCircle = OnB2DrawCircle;
    m_b2DebugDraw.DrawSolidCircle = OnB2DrawSolidCircle;
    m_b2DebugDraw.DrawSolidCapsule = OnB2DrawSolidCapsule;
    m_b2DebugDraw.DrawSegment = OnB2DrawSegment;
    m_b2DebugDraw.DrawTransform = OnB2DrawTransform;
    m_b2DebugDraw.DrawPoint = OnB2DrawPoint;
}

void Physics2D::Clean() const {
    if (!b2World_IsValid(m_worldId)) {
        LOG_ERROR("Trying to destroy an invalid physics world");
        return;
    }

    b2DestroyWorld(m_worldId);
    LOG_TRACE("Destroyed physics world");
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

void Physics2D::SyncDebugDrawFlags() {
    m_b2DebugDraw.drawShapes = m_debugDrawFnc.drawShapes;
    m_b2DebugDraw.drawJoints = m_debugDrawFnc.drawJoints;
    m_b2DebugDraw.drawAABBs = m_debugDrawFnc.drawAABBs;
    m_b2DebugDraw.drawMass = m_debugDrawFnc.drawMass;
    m_b2DebugDraw.drawContacts = m_debugDrawFnc.drawContacts;
}

void Physics2D::UpdateDebugDraw() {
    if (isDebugDraw) {
        SyncDebugDrawFlags();
        b2World_Draw(m_worldId, &m_b2DebugDraw);
    }
}

DebugDrawFnc &Physics2D::GetDebugDrawFnc() { return m_debugDrawFnc; }

} // namespace Aeon
