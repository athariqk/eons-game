#pragma once

#include <ncore.hpp>
#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>

// Demo spin component.
NC_COMPONENT( TestSpin )
{
    float rotation       = 0;
    bool switch_rot      = true;
    nc::Quaternion start = nc::Quaternion( 180, nc::Vec3::up() );
    nc::Quaternion end   = nc::Quaternion( 0, nc::Vec3::up() );

    NC_PROPS_BEGIN()
    ADD_PROPERTY( rotation )
    ADD_PROPERTY( switch_rot )
    ADD_PROPERTY( start )
    ADD_PROPERTY( end )
    NC_PROPS_END()
};
