#pragma once

#include <Component.h>

#include "Genes.h"

class SpeciesComponent;

/**
 * @brief Component representing an organism in the simulation
 */
class OrganismComponent : public Aeon::Component {
public:
    OrganismComponent(SpeciesComponent *species);
    OrganismComponent(const OrganismComponent &organism) : speciesId(organism.speciesId), genome(organism.genome) {}
    ~OrganismComponent() override {}

    // Data members
    size_t speciesId = 0;
    Genes genome{};
    double fitness = 0.0;
    float curEnergy = 0.0f;
};
