#pragma once

#include <modules/ecs/Component.h>

#include <microcosmos/Genes.h>

class SpeciesComponent;

/**
 * @brief Component representing an organism in the simulation
 */
class OrganismComponent : public ncore::Component {
public:
    OrganismComponent(SpeciesComponent *species);
    OrganismComponent(const OrganismComponent &organism) : species_id(organism.species_id), genome(organism.genome) {}
    ~OrganismComponent() override {}

    // Data members
    size_t species_id = 0;
    Genes genome{};
    double fitness = 0.0;
    float cur_energy = 0.0f;
};
