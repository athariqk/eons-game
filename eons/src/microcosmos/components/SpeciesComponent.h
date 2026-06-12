#pragma once

#include <string>
#include <vector>

#include <modules/ecs/Entity.h>

#include <microcosmos/Genes.h>

class OrganismComponent;

/**
 * @brief Component representing a species in the simulation
 *
 * This is pure data - all logic is handled by SpeciesSystem
 * Equivalent to a GA Population operator
 */
class SpeciesComponent : public ncore::Component {
public:
    SpeciesComponent() = default;
    SpeciesComponent(std::string name, std::string genus, std::string epithet);
    ~SpeciesComponent() override {}

    // Data members
    int population_count = 0;
    int age = 0;
    std::string name;
    std::string genus;
    std::string epithet;
    Genes genes{};
    int32_t generation = 0;

    [[nodiscard]] std::string get_name_formatted(bool identifier) const;
};
