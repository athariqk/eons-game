#pragma once

#include <string>

#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>

enum class BehaviourState {
    IDLING     = 0,
    RUN_TUMBLE = 1,
    ABSORBING  = 2,
    EVALUATE   = 3
};

NENUM(
    BehaviourState, NENUM_ELEMENT( BehaviourState, IDLING ), NENUM_ELEMENT( BehaviourState, RUN_TUMBLE ),
    NENUM_ELEMENT( BehaviourState, ABSORBING ), NENUM_ELEMENT( BehaviourState, EVALUATE )
);

NC_COMPONENT( OrganismAIComponent )
{
    OrganismAIComponent() : move_speed( 0.0f ), act_interval( 10.0f ) {}
    OrganismAIComponent( float p_speed, float p_think_interval ) :
        move_speed( p_speed ), act_interval( p_think_interval )
    {}

    BehaviourState state = BehaviourState::IDLING;
    float move_speed;
    float absorb_speed       = 0.2f;
    bool is_food_found       = false;
    bool has_moved           = false;
    bool is_absorbing        = false;
    bool reproduced          = false;
    float act_interval       = 10.0f;
    float moving_interval    = 10.0f;
    float act_timer          = 0.0f;
    float reproduce_interval = 0.0f;

    std::string get_current_behavior() const;

    NC_PROPS_BEGIN()
    ADD_PROPERTY( state )
    ADD_PROPERTY( move_speed )
    ADD_PROPERTY( absorb_speed )
    ADD_PROPERTY( is_food_found )
    ADD_PROPERTY( has_moved )
    ADD_PROPERTY( is_absorbing )
    ADD_PROPERTY( reproduced )
    ADD_PROPERTY( act_interval )
    ADD_PROPERTY( moving_interval )
    ADD_PROPERTY( act_timer )
    ADD_PROPERTY( reproduce_interval )
    NC_PROPS_END()
};
