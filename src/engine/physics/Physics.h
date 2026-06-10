#pragma once

#include <Vec2D.h>
#include "box2d/box2d.h"

namespace ncore {

struct DebugDrawFnc {
    void (*draw_polygon)(const Vec2D *vertices, int vertexCount, uint32_t color, void *context) = nullptr;
    void (*draw_solid_polygon)(const Vec2D *vertices, int vertexCount, float radius, uint32_t color,
                               void *context) = nullptr;
    void (*draw_circle)(Vec2D center, float radius, uint32_t color, void *context) = nullptr;
    void (*draw_solid_circle)(Vec2D center, float radius, uint32_t color, void *context) = nullptr;
    void (*draw_solid_capsule)(Vec2D p1, Vec2D p2, float radius, uint32_t color, void *context) = nullptr;
    void (*draw_segment)(Vec2D p1, Vec2D p2, uint32_t color, void *context) = nullptr;
    void (*draw_transform)(Vec2D position, float angle, void *context) = nullptr;
    void (*draw_point)(Vec2D p, float size, uint32_t color, void *context) = nullptr;

    void *context = nullptr;

    bool draw_shapes = true;
    bool draw_joints = true;
    bool draw_aabbs = false;
    bool draw_mass = false;
    bool draw_contacts = false;
};

/**
 * @brief The engine component that handles 2D physics.
 */
class Physics2D {
public:
    Physics2D() {}
    ~Physics2D() = default;

    void init();
    void step() const;
    void clean() const;

    float get_time_step() const { return time_step; }
    void set_time_step(float value) { time_step = value; }

    int get_sub_step_count() const { return sub_step_count; }
    void set_sub_step_count(int value) { sub_step_count = value; }

    b2BodyId create_body(const b2BodyDef *bodyDef) const;
    void destroy_body(const b2BodyId &bodyId) const;

	// Queries
    bool is_body_valid(const b2BodyId &bodyId) const;
    bool is_body_awake(const b2BodyId &bodyId) const;
    Vec2D get_body_position(const b2BodyId &bodyId) const;
    float get_body_angle(const b2BodyId &bodyId) const;
    Vec2D get_body_velocity(const b2BodyId &bodyId) const;

	// Setters
    void apply_linear_impulse(const b2BodyId &bodyId, const Vec2D &impulse) const;
    void apply_linear_force(const b2BodyId &bodyId, const Vec2D &force) const;

    void update_debug_draw();
    DebugDrawFnc &get_debug_draw_fnc();

    bool is_debug_draw = false;

private:
    b2WorldId world_id{0};
    b2DebugDraw b2_debug_draw{0};

    DebugDrawFnc debug_draw_fnc;

    float time_step = 1.0f / 60.0f;
    int sub_step_count = 4;

    void sync_debug_draw_flags();
};

} // namespace ncore
