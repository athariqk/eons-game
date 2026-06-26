#pragma once

#include <ncore/kernel/structures.h>
#include <ncore/kernel/types.h>

struct FoodComponent {
    FoodComponent() : cur_energy( 0.0f ) {}
    explicit FoodComponent( const float energy ) : cur_energy( energy ) {}

    float cur_energy;
    bool caught = false;
    ncore::Vec2 eater_pos;

    NSTRUCT(
        FoodComponent, NC_F( FoodComponent, cur_energy ) NC_F( FoodComponent, caught ) NC_F( FoodComponent, eater_pos )
    )
};
