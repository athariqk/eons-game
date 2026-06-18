#pragma once

#include <microcosmos/Genes.h>

class SpeciesComponent;

class OrganismComponent {

public:
    OrganismComponent() = default;
    OrganismComponent(SpeciesComponent *species);
    OrganismComponent(const OrganismComponent &organism) = default;

    size_t species_id = 0;
    Genes genome{};
    double fitness = 0.0;
    float cur_energy = 0.0f;
};
