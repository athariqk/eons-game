#include "Physics.h"

#include <Logger.h>
#include <Vec2D.h>
#include <cmath>

namespace ncore {

#define ENSURE_WORLD()                                                                                                 \
    if (!b2World_IsValid(world_id)) {                                                                                  \
        LOG_ERROR(log::PHYSICS, "World is invalid");                                                                   \
        return;                                                                                                        \
    }

#define ENSURE_BODY_RETURN(bodyId, ret)                                                                                \
    if (!is_body_valid(bodyId)) {                                                                                      \
        LOG_ERROR(log::PHYSICS, "Body is invalid");                                                                    \
        return ret;                                                                                                    \
    }

// ---- Forwarding callbacks: b2DebugDraw -> DebugDrawFnc ----

static void OnB2DrawPolygon(const b2Vec2 *vertices, int vertexCount, b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->draw_polygon)
        return;
    Vec2D buf[8];
    int n = vertexCount < 8 ? vertexCount : 8;
    for (int i = 0; i < n; i++)
        buf[i] = Vec2D(vertices[i].x, vertices[i].y);
    wrapper->draw_polygon(buf, n, static_cast<uint32_t>(color), wrapper->context);
}

static void OnB2DrawSolidPolygon(b2Transform transform, const b2Vec2 *vertices, int vertexCount, float radius,
                                 b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->draw_solid_polygon)
        return;
    Vec2D buf[8];
    int n = vertexCount < 8 ? vertexCount : 8;
    for (int i = 0; i < n; i++) {
        b2Vec2 wp = b2TransformPoint(transform, vertices[i]);
        buf[i] = Vec2D(wp.x, wp.y);
    }
    wrapper->draw_solid_polygon(buf, n, radius, static_cast<uint32_t>(color), wrapper->context);
}

static void OnB2DrawCircle(b2Vec2 center, float radius, b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->draw_circle)
        return;
    wrapper->draw_circle(Vec2D(center.x, center.y), radius, static_cast<uint32_t>(color), wrapper->context);
}

static void OnB2DrawSolidCircle(b2Transform transform, float radius, b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->draw_solid_circle)
        return;
    wrapper->draw_solid_circle(Vec2D(transform.p.x, transform.p.y), radius, static_cast<uint32_t>(color),
                               wrapper->context);
}

static void OnB2DrawSolidCapsule(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->draw_solid_capsule)
        return;
    wrapper->draw_solid_capsule(Vec2D(p1.x, p1.y), Vec2D(p2.x, p2.y), radius, static_cast<uint32_t>(color),
                                wrapper->context);
}

static void OnB2DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->draw_segment)
        return;
    wrapper->draw_segment(Vec2D(p1.x, p1.y), Vec2D(p2.x, p2.y), static_cast<uint32_t>(color), wrapper->context);
}

static void OnB2DrawTransform(b2Transform transform, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->draw_transform)
        return;
    float angle = b2Rot_GetAngle(transform.q);
    wrapper->draw_transform(Vec2D(transform.p.x, transform.p.y), angle, wrapper->context);
}

static void OnB2DrawPoint(b2Vec2 p, float size, b2HexColor color, void *ctx) {
    auto *wrapper = static_cast<DebugDrawFnc *>(ctx);
    if (!wrapper->draw_point)
        return;
    wrapper->draw_point(Vec2D(p.x, p.y), size, static_cast<uint32_t>(color), wrapper->context);
}

// ---------- IMPL ------

void Physics2D::init() {
    if (b2World_IsValid(world_id))
        return;

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity.y = 0;
    world_id = b2CreateWorld(&worldDef);

    b2_debug_draw = b2DefaultDebugDraw();
    b2_debug_draw.context = &debug_draw_fnc;
    b2_debug_draw.DrawPolygon = OnB2DrawPolygon;
    b2_debug_draw.DrawSolidPolygon = OnB2DrawSolidPolygon;
    b2_debug_draw.DrawCircle = OnB2DrawCircle;
    b2_debug_draw.DrawSolidCircle = OnB2DrawSolidCircle;
    b2_debug_draw.DrawSolidCapsule = OnB2DrawSolidCapsule;
    b2_debug_draw.DrawSegment = OnB2DrawSegment;
    b2_debug_draw.DrawTransform = OnB2DrawTransform;
    b2_debug_draw.DrawPoint = OnB2DrawPoint;
}

void Physics2D::step() const {
    ENSURE_WORLD()
    b2World_Step(world_id, time_step, sub_step_count);
}

void Physics2D::clean() const {
    ENSURE_WORLD()
    b2DestroyWorld(world_id);
    LOG_TRACE(log::PHYSICS, "Destroyed physics world");
}

b2BodyId Physics2D::create_body(const b2BodyDef *bodyDef) const {
    if (!b2World_IsValid(world_id))
        return {0};
    return b2CreateBody(world_id, bodyDef);
}

void Physics2D::destroy_body(const b2BodyId &bodyId) const {
    ENSURE_WORLD()
    if (!is_body_valid(bodyId)) {
        LOG_ERROR(log::PHYSICS, "Body is invalid");
        return;
    }
    b2DestroyBody(bodyId);
}

bool Physics2D::is_body_valid(const b2BodyId &bodyId) const { return b2Body_IsValid(bodyId); }

bool Physics2D::is_body_awake(const b2BodyId &bodyId) const { return b2Body_IsAwake(bodyId); }

Vec2D Physics2D::get_body_position(const b2BodyId &bodyId) const {
    ENSURE_BODY_RETURN(bodyId, Vec2D())
    auto pos = b2Body_GetPosition(bodyId);
    return Vec2D(pos.x, pos.y);
}

float Physics2D::get_body_angle(const b2BodyId &bodyId) const {
    ENSURE_BODY_RETURN(bodyId, 0.0f)
    return b2Rot_GetAngle(b2Body_GetRotation(bodyId));
}

Vec2D Physics2D::get_body_velocity(const b2BodyId &bodyId) const {
    ENSURE_BODY_RETURN(bodyId, Vec2D())
    auto vel = b2Body_GetLinearVelocity(bodyId);
    return Vec2D(vel.x, vel.y);
}

void Physics2D::apply_linear_impulse(const b2BodyId &bodyId, const Vec2D &impulse) const {
    if (!is_body_valid(bodyId)) {
        LOG_ERROR(log::PHYSICS, "Can't apply impulse on a uninitialized body");
        return;
    }
    b2Body_ApplyLinearImpulseToCenter(bodyId, b2Vec2(impulse.x, impulse.y), true);
}

void Physics2D::apply_linear_force(const b2BodyId &bodyId, const Vec2D &force) const {
    if (!is_body_valid(bodyId)) {
        LOG_ERROR(log::PHYSICS, "Can't apply force on a uninitialized body");
        return;
    }
    b2Body_ApplyForceToCenter(bodyId, b2Vec2(force.x, force.y), true);
}

void Physics2D::sync_debug_draw_flags() {
    b2_debug_draw.drawShapes = debug_draw_fnc.draw_shapes;
    b2_debug_draw.drawJoints = debug_draw_fnc.draw_joints;
    b2_debug_draw.drawAABBs = debug_draw_fnc.draw_aabbs;
    b2_debug_draw.drawMass = debug_draw_fnc.draw_mass;
    b2_debug_draw.drawContacts = debug_draw_fnc.draw_contacts;
}

void Physics2D::update_debug_draw() {
    if (is_debug_draw) {
        sync_debug_draw_flags();
        b2World_Draw(world_id, &b2_debug_draw);
    }
}

DebugDrawFnc &Physics2D::get_debug_draw_fnc() { return debug_draw_fnc; }

} // namespace ncore
