#pragma once

#include <string>

#include <microcosmos/Genes.h>

struct SpeciesComponent {
    SpeciesComponent() = default;
    SpeciesComponent(std::string name, std::string genus, std::string epithet) :
        name(std::move(name)), genus(std::move(genus)), epithet(std::move(epithet))
    {}

    int population_count = 0;
    int age              = 0;
    std::string name;
    std::string genus;
    std::string epithet;
    Genes genes{};
    int32_t generation = 0;

    [[nodiscard]] std::string get_name_formatted(bool identifier) const;

    NSTRUCT(
        SpeciesComponent, NC_F(SpeciesComponent, population_count) NC_F(SpeciesComponent, age)
                              NC_F(SpeciesComponent, name) NC_F(SpeciesComponent, genus) NC_F(SpeciesComponent, epithet)
                                  NC_F(SpeciesComponent, genes) NC_F(SpeciesComponent, generation)
    )
};
