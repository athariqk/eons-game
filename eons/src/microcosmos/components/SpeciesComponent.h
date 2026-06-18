#pragma once

#include <string>
#include <vector>

#include <microcosmos/Genes.h>

class OrganismComponent;

class SpeciesComponent {

public:
    SpeciesComponent() = default;
    SpeciesComponent(std::string name, std::string genus, std::string epithet) :
        name(std::move(name)), genus(std::move(genus)), epithet(std::move(epithet)) {}

    int population_count = 0;
    int age = 0;
    std::string name;
    std::string genus;
    std::string epithet;
    Genes genes{};
    int32_t generation = 0;

    [[nodiscard]] std::string get_name_formatted(bool identifier) const;
};
