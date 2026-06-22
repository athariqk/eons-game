#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <shared_mutex>

struct SpeciesComponent;
struct OrganismComponent;

/**
 * REFACTOR: this is a wrong way to do things...
 */
class SpeciesRegistry {
public:
    // Registration
    void track_species(SpeciesComponent &species);
    void untrack_species(SpeciesComponent *species);
    void track_organism(OrganismComponent &organism);
    void untrack_organism(OrganismComponent *organism);
    void clear_organisms(SpeciesComponent *species);

    // Queries
    SpeciesComponent *get_species_by_id(size_t entity_id) const;
    SpeciesComponent *get_species(const SpeciesComponent *species) const;
    std::string get_species_name(const OrganismComponent *organism) const;
    int get_species_idx(const SpeciesComponent *species) const;

    // Access
    auto &all_species() { return species_reg_; }
    auto &all_organisms() { return organism_reg_; }
    const auto &all_species() const { return species_reg_; }
    const auto &all_organisms() const { return organism_reg_; }

private:
    std::vector<SpeciesComponent *> species_reg_;
    std::vector<OrganismComponent *> organism_reg_;
    mutable std::shared_mutex mutex_;
};
