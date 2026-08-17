#pragma once

#include <ncore/core/types.h>
#include <ncore/core/vector.h>

struct FoodComponent {
    FoodComponent() : cur_energy( 0.0f ) {}
    explicit FoodComponent( const float energy ) : cur_energy( energy ) {}

    float cur_energy;
    bool caught = false;
    nc::Vec2 eater_pos;

    NSTRUCTV(
        FoodComponent, NC_F( FoodComponent, cur_energy ), NC_F( FoodComponent, caught ),
        NC_F( FoodComponent, eater_pos )
    )
};
