#pragma once

#include <ncore/kernel/resource.h>
#include <ncore/kernel/structures.h>
#include <ncore/modules/service.h>

#include "physics_body.h"

namespace ncore {

struct DebugDrawFnc {
    void (*draw_polygon)(const Vec2 *vertices, int vertexCount, uint32_t color, void *context) = nullptr;
    void (*draw_solid_polygon)(const Vec2 *vertices, int vertexCount, float radius, uint32_t color,
                               void *context) = nullptr;
    void (*draw_circle)(Vec2 center, float radius, uint32_t color, void *context) = nullptr;
    void (*draw_solid_circle)(Vec2 center, float radius, uint32_t color, void *context) = nullptr;
    void (*draw_solid_capsule)(Vec2 p1, Vec2 p2, float radius, uint32_t color, void *context) = nullptr;
    void (*draw_segment)(Vec2 p1, Vec2 p2, uint32_t color, void *context) = nullptr;
    void (*draw_transform)(Vec2 position, float angle, void *context) = nullptr;
    void (*draw_point)(Vec2 p, float size, uint32_t color, void *context) = nullptr;
    void *context = nullptr;
    bool draw_shapes = true;
    bool draw_joints = true;
    bool draw_aabbs = false;
    bool draw_mass = false;
    bool draw_contacts = false;
};

class IPhysicsService : public IService {
    NCLASS(IPhysicsService, IService)

public:
    virtual void step() const = 0;

    float get_time_step() const { return time_step; }
    void set_time_step(float value) { time_step = value; }

    int get_sub_step_count() const { return sub_step_count; }
    void set_sub_step_count(int value) { sub_step_count = value; }

    virtual RID create_shape(ShapeType type) = 0;

    virtual RID create_rigidbody() = 0;
    virtual RID create_softbody() = 0;
    virtual void destroy_body(RID body) = 0;

    virtual bool is_body_valid(RID body) const = 0;
    virtual bool is_body_awake(RID body) const = 0;
    virtual Vec2 get_body_position(RID body) const = 0;
    virtual float get_body_angle(RID body) const = 0;
    virtual Vec2 get_body_velocity(RID body) const = 0;

    virtual void apply_linear_impulse(RID body, const Vec2 &impulse) = 0;
    virtual void apply_linear_force(RID body, const Vec2 &force) = 0;

    virtual void update_debug_draw() = 0;
    virtual DebugDrawFnc &get_debug_draw_fnc() = 0;

    bool is_debug_draw = false;

protected:
    float time_step = 1.0f / 60.0f;
    int sub_step_count = 4;
};

} // namespace ncore
