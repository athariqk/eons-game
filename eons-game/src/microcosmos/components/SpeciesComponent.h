#pragma once

#include <string>

#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>

#include <microcosmos/Genes.h>

NC_COMPONENT( SpeciesComponent )
{
    SpeciesComponent() = default;
    SpeciesComponent( std::string name, std::string genus, std::string epithet ) :
        name( std::move( name ) ), genus( std::move( genus ) ), epithet( std::move( epithet ) )
    {}

    int population_count = 0;
    int age              = 0;
    std::string name;
    std::string genus;
    std::string epithet;
    Genes genes{};
    int32_t generation = 0;

    [[nodiscard]] std::string get_name_formatted( bool identifier ) const;

    NC_PROPS_BEGIN()
    ADD_PROPERTY( population_count )
    ADD_PROPERTY( age )
    ADD_PROPERTY( name )
    ADD_PROPERTY( genus )
    ADD_PROPERTY( epithet )
    ADD_PROPERTY( genes )
    ADD_PROPERTY( generation )
    NC_PROPS_END()
};
