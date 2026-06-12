#include "OrganismComponent.h"

#include "SpeciesComponent.h"

OrganismComponent::OrganismComponent(SpeciesComponent *species) :
    species_id(species->entity->get_id()), genome(species->genes) {}
