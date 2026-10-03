#pragma once

#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>

#include <microcosmos/Genes.h>

NC_COMPONENT( OrganismComponent )
{
    OrganismComponent()                                    = default;
    OrganismComponent( const OrganismComponent& organism ) = default;

    size_t species_id = 0;
    Genes genome{};
    double fitness   = 0.0;
    float cur_energy = 0.0f;

    NC_PROPS_BEGIN()
    ADD_PROPERTY( species_id )
    ADD_PROPERTY( genome )
    ADD_PROPERTY( fitness )
    ADD_PROPERTY( cur_energy )
    NC_PROPS_END()
};
