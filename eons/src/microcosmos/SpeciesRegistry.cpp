#include "SpeciesRegistry.h"

#include <algorithm>
#include <ranges>

#include <microcosmos/components/OrganismComponent.h>
#include <microcosmos/components/SpeciesComponent.h>

void SpeciesRegistry::track_species(SpeciesComponent& species)
{
    std::unique_lock lock(mutex_);
    species_reg_.push_back(&species);
}

void SpeciesRegistry::untrack_species(SpeciesComponent* species)
{
    std::unique_lock lock(mutex_);
    auto it = std::ranges::find(species_reg_, species);
    if (it != species_reg_.end())
        species_reg_.erase(it);
}

void SpeciesRegistry::track_organism(OrganismComponent& organism)
{
    std::unique_lock lock(mutex_);
    organism_reg_.push_back(&organism);
}

void SpeciesRegistry::untrack_organism(OrganismComponent* organism)
{
    std::unique_lock lock(mutex_);
    auto it = std::ranges::find(organism_reg_, organism);
    if (it != organism_reg_.end())
        organism_reg_.erase(it);
}

void SpeciesRegistry::clear_organisms(SpeciesComponent* species)
{
    std::unique_lock lock(mutex_);
    organism_reg_.clear();
}

SpeciesComponent* SpeciesRegistry::get_species_by_id(size_t entity_id) const
{
    // std::shared_lock lock(mutex_);
    // for (auto *species: species_reg_) {
    //     if (species->entity && species->entity->id == entity_id)
    //         return species;
    // }
    return nullptr;
}

SpeciesComponent* SpeciesRegistry::get_species(const SpeciesComponent* species) const
{
    // return species->entity ? get_species_by_id(species->entity->id) : nullptr;
    return nullptr;
}

std::string SpeciesRegistry::get_species_name(const OrganismComponent* organism) const
{
    auto* species = get_species_by_id(organism->species_id);
    return species ? species->get_name_formatted(false) : "Unknown";
}

int SpeciesRegistry::get_species_idx(const SpeciesComponent* species) const
{
    std::shared_lock lock(mutex_);
    auto it = std::ranges::find(species_reg_, species);
    if (it != species_reg_.end())
        return static_cast<int>(std::distance(species_reg_.begin(), it));
    return -1;
}
