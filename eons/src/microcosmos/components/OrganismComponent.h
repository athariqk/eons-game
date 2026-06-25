#pragma once

#include <microcosmos/Genes.h>

struct OrganismComponent {
    OrganismComponent()                                  = default;
    OrganismComponent(const OrganismComponent& organism) = default;

    size_t species_id = 0;
    Genes genome{};
    double fitness   = 0.0;
    float cur_energy = 0.0f;

    NSTRUCT(
        OrganismComponent, NC_F(OrganismComponent, species_id) NC_F(OrganismComponent, genome)
                               NC_F(OrganismComponent, fitness) NC_F(OrganismComponent, cur_energy)
    )
};
