#include "OrganismComponent.h"

#include "SpeciesComponent.h"

OrganismComponent::OrganismComponent(SpeciesComponent *species) : species_id(0), genome(species->genes) {}
