#pragma once

#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/runtime/ecs/ecs_component.h>

NC_COMPONENT( FoodComponent )
{
    FoodComponent() : cur_energy( 0.0f ) {}
    explicit FoodComponent( const float energy ) : cur_energy( energy ) {}

    float cur_energy;
    bool caught = false;
    nc::Vec2f eater_pos;

    NC_PROPS_BEGIN()
    ADD_PROPERTY( cur_energy )
    ADD_PROPERTY( caught )
    ADD_PROPERTY( eater_pos )
    NC_PROPS_END()
};
