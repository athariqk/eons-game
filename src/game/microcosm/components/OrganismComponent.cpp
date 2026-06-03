#include "OrganismComponent.h"

#include "SpeciesComponent.h"

OrganismComponent::OrganismComponent(SpeciesComponent *species) :
    speciesId(species->entity->GetID()), genome(species->genes) {}
