#pragma once

#include <ncore/kernel/rid.h>
#include <ncore/kernel/structures.h>
#include <ncore/modules/module.h>

#include "debug_draw.h"
#include "physics_body.h"

namespace nc {

/**
 * @brief IPhysicsModule defines base functionalities for physics
 * simulation and operations.
 */
class IPhysicsModule : public IModule {
    NCLASS( IPhysicsModule, IModule )

public:
    virtual void step() const = 0;

    float get_time_step() const
    {
        return time_step;
    }
    void set_time_step( float value )
    {
        time_step = value;
    }

    int get_sub_step_count() const
    {
        return sub_step_count;
    }
    void set_sub_step_count( int value )
    {
        sub_step_count = value;
    }

    virtual RID create_shape( ShapeType type ) = 0;

    virtual RID create_rigidbody()        = 0;
    virtual RID create_softbody()         = 0;
    virtual void destroy_body( RID body ) = 0;

    virtual bool is_body_valid( RID body ) const     = 0;
    virtual bool is_body_awake( RID body ) const     = 0;
    virtual Vec2 get_body_position( RID body ) const = 0;
    virtual float get_body_angle( RID body ) const   = 0;
    virtual Vec2 get_body_velocity( RID body ) const = 0;

    virtual void apply_linear_impulse( RID body, const Vec2& impulse ) = 0;
    virtual void apply_linear_force( RID body, const Vec2& force )     = 0;

    virtual void update_debug_draw()           = 0;
    virtual PhysicsDebugDraw& get_debug_draw() = 0;

    bool is_debug_draw = false;

protected:
    float time_step    = 1.0f / 60.0f;
    int sub_step_count = 4;
};

} // namespace nc
