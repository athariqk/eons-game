#include "box2d_physics_impl.h"

#include <box2d/box2d.h>

#include <ncore/modules/module_registry.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

namespace nc {

static void OnB2DrawPolygon( const b2Vec2* vertices, int vertexCount, b2HexColor color, void* ctx )
{
    auto* wrapper = static_cast<PhysicsDebugDraw*>( ctx );
    if (!wrapper->draw_polygon)
        return;
    Vec2 buf[8];
    int n = vertexCount < 8 ? vertexCount : 8;
    for (int i = 0; i < n; i++)
        buf[i] = Vec2( vertices[i].x, vertices[i].y );
    wrapper->draw_polygon( buf, n, static_cast<uint32_t>( color ), wrapper->context );
}

static void OnB2DrawSolidPolygon(
    b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor color, void* ctx
)
{
    auto* wrapper = static_cast<PhysicsDebugDraw*>( ctx );
    if (!wrapper->draw_solid_polygon)
        return;
    Vec2 buf[8];
    int n = vertexCount < 8 ? vertexCount : 8;
    for (int i = 0; i < n; i++) {
        b2Vec2 wp = b2TransformPoint( transform, vertices[i] );
        buf[i]    = Vec2( wp.x, wp.y );
    }
    wrapper->draw_solid_polygon( buf, n, radius, static_cast<uint32_t>( color ), wrapper->context );
}

static void OnB2DrawCircle( b2Vec2 center, float radius, b2HexColor color, void* ctx )
{
    auto* wrapper = static_cast<PhysicsDebugDraw*>( ctx );
    if (!wrapper->draw_circle)
        return;
    wrapper->draw_circle( Vec2( center.x, center.y ), radius, static_cast<uint32_t>( color ), wrapper->context );
}

static void OnB2DrawSolidCircle( b2Transform transform, float radius, b2HexColor color, void* ctx )
{
    auto* wrapper = static_cast<PhysicsDebugDraw*>( ctx );
    if (!wrapper->draw_solid_circle)
        return;
    wrapper->draw_solid_circle(
        Vec2( transform.p.x, transform.p.y ), radius, static_cast<uint32_t>( color ), wrapper->context
    );
}

static void OnB2DrawSolidCapsule( b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void* ctx )
{
    auto* wrapper = static_cast<PhysicsDebugDraw*>( ctx );
    if (!wrapper->draw_solid_capsule)
        return;
    wrapper->draw_solid_capsule(
        Vec2( p1.x, p1.y ), Vec2( p2.x, p2.y ), radius, static_cast<uint32_t>( color ), wrapper->context
    );
}

static void OnB2DrawSegment( b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* ctx )
{
    auto* wrapper = static_cast<PhysicsDebugDraw*>( ctx );
    if (!wrapper->draw_segment)
        return;
    wrapper->draw_segment( Vec2( p1.x, p1.y ), Vec2( p2.x, p2.y ), static_cast<uint32_t>( color ), wrapper->context );
}

static void OnB2DrawTransform( b2Transform transform, void* ctx )
{
    auto* wrapper = static_cast<PhysicsDebugDraw*>( ctx );
    if (!wrapper->draw_transform)
        return;
    float angle = b2Rot_GetAngle( transform.q );
    wrapper->draw_transform( Vec2( transform.p.x, transform.p.y ), angle, wrapper->context );
}

static void OnB2DrawPoint( b2Vec2 p, float size, b2HexColor color, void* ctx )
{
    auto* wrapper = static_cast<PhysicsDebugDraw*>( ctx );
    if (!wrapper->draw_point)
        return;
    wrapper->draw_point( Vec2( p.x, p.y ), size, static_cast<uint32_t>( color ), wrapper->context );
}

Error Box2DPhysicsImpl::init()
{
    if (b2World_IsValid( world_id ))
        return Error::FAIL;

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity.y  = 0;
    world_id            = b2CreateWorld( &worldDef );

    b2_debug_draw         = b2DefaultDebugDraw();
    b2_debug_draw.context = &debug_draw;
    // b2_debug_draw.DrawPolygonFcn      = OnB2DrawPolygon;
    b2_debug_draw.DrawSolidPolygonFcn = OnB2DrawSolidPolygon;
    b2_debug_draw.DrawCircleFcn       = OnB2DrawCircle;
    // b2_debug_draw.DrawSolidCircleFcn  = OnB2DrawSolidCircle;
    b2_debug_draw.DrawSolidCapsuleFcn = OnB2DrawSolidCapsule;
    // b2_debug_draw.DrawSegment         = OnB2DrawSegment;
    b2_debug_draw.DrawTransformFcn = OnB2DrawTransform;
    b2_debug_draw.DrawPointFcn     = OnB2DrawPoint;

    return Error::OK;
}

void Box2DPhysicsImpl::step() const
{
    NC_ASSERT_RET( b2World_IsValid( world_id ), "Physics world is not initialized" );
    b2World_Step( world_id, time_step, sub_step_count );
}

void Box2DPhysicsImpl::finalize()
{
    NC_ASSERT_RET( b2World_IsValid( world_id ), "Physics world is not initialized" );
    b2DestroyWorld( world_id );
    NC_LOG_TRACE_C( log::PHYSICS, "Destroyed physics world" );
}

RID Box2DPhysicsImpl::create_shape( ShapeType type )
{
    NC_ASSERT_RETVAL( b2World_IsValid( world_id ), RID(), "Physics world is not initialized" );
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density    = 1.0f;
    // shapeDef.friction   = 0.3f;
    return { 0 };
}

RID Box2DPhysicsImpl::create_rigidbody()
{
    NC_ASSERT_RETVAL( b2World_IsValid( world_id ), RID(), "Physics world is not initialized" );
    b2BodyDef bodyDef     = b2DefaultBodyDef();
    bodyDef.type          = b2_dynamicBody;
    bodyDef.position      = b2Vec2{ 0, 0 };
    bodyDef.linearDamping = 2.5f;
    auto b2body           = b2CreateBody( world_id, &bodyDef );
    auto rid              = RID{ static_cast<uint64_t>( b2body.index1 ) };
    body_map[rid]         = b2body;
    return rid;
}

RID Box2DPhysicsImpl::create_softbody()
{
    return create_rigidbody(); // softbody uses same physics for now
}

void Box2DPhysicsImpl::destroy_body( RID body )
{
    NC_ASSERT_RET( b2World_IsValid( world_id ), "Physics world is not initialized" );
    NC_ASSERT_RET( is_body_valid( body ), "Body is invalid" );
    auto b2id = body_map.at( body );
    b2DestroyBody( b2id );
    body_map.erase( body );
}

bool Box2DPhysicsImpl::is_body_valid( RID body ) const
{
    return body_map.find( body ) != body_map.end();
}

bool Box2DPhysicsImpl::is_body_awake( RID body ) const
{
    NC_ASSERT_RETVAL( is_body_valid( body ), false, "Body is invalid" );
    return b2Body_IsAwake( body_map.at( body ) );
}

Vec2 Box2DPhysicsImpl::get_body_position( RID body ) const
{
    NC_ASSERT_RETVAL( is_body_valid( body ), Vec2(), "Body is invalid" );
    auto pos = b2Body_GetPosition( body_map.at( body ) );
    return Vec2( pos.x, pos.y );
}

float Box2DPhysicsImpl::get_body_angle( RID body ) const
{
    NC_ASSERT_RETVAL( is_body_valid( body ), 0.0f, "Body is invalid" );
    return b2Rot_GetAngle( b2Body_GetRotation( body_map.at( body ) ) );
}

Vec2 Box2DPhysicsImpl::get_body_velocity( RID body ) const
{
    NC_ASSERT_RETVAL( is_body_valid( body ), Vec2(), "Body is invalid" );
    auto vel = b2Body_GetLinearVelocity( body_map.at( body ) );
    return Vec2( vel.x, vel.y );
}

void Box2DPhysicsImpl::apply_linear_impulse( RID body, const Vec2& impulse )
{
    NC_ASSERT_RET( is_body_valid( body ), "Body is invalid" );
    b2Body_ApplyLinearImpulseToCenter( body_map.at( body ), b2Vec2{ impulse.X, impulse.Y }, true );
}

void Box2DPhysicsImpl::apply_linear_force( RID body, const Vec2& force )
{
    NC_ASSERT_RET( is_body_valid( body ), "Body is invalid" );
    b2Body_ApplyForceToCenter( body_map.at( body ), b2Vec2{ force.X, force.Y }, true );
}

void Box2DPhysicsImpl::sync_debug_draw_flags()
{
    b2_debug_draw.drawShapes   = debug_draw.draw_shapes;
    b2_debug_draw.drawJoints   = debug_draw.draw_joints;
    b2_debug_draw.drawBounds   = debug_draw.draw_aabbs;
    b2_debug_draw.drawMass     = debug_draw.draw_mass;
    b2_debug_draw.drawContacts = debug_draw.draw_contacts;
}

void Box2DPhysicsImpl::update_debug_draw()
{
    if (is_debug_draw) {
        sync_debug_draw_flags();
        b2World_Draw( world_id, &b2_debug_draw );
    }
}

PhysicsDebugDraw& Box2DPhysicsImpl::get_debug_draw()
{
    return debug_draw;
}

} // namespace nc
