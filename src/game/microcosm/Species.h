#pragma once

#include "Genes.h"

#include "EntitySystem.h"

#include <string>
#include <vector>

class OrganismComponent;

/**
 * Equivalent to a GA Population operator
 */
class Species : public Component {
public:
    Species() = default;
    Species(std::string name, std::string genus, std::string epithet);
    ~Species() override {}

    void OnUpdate(float delta) override;

    //! \brief Spawn a single organism
    void addOrganism();

    //! \brief Spawn a single organism with given genes
    //! and randomly mutate it if set true
    void addOrganism(Genes &genes, bool mutate);

    //! \brief Destroy a single given organism
    void deleteOrganism(OrganismComponent *organism);

    //! \brief Destroy all members of this species
    void clearOrganisms();

    [[nodiscard]] size_t getID() const;
    [[nodiscard]] size_t getPopulationCount() const;
    int getOrganismIndex(OrganismComponent *organism);
    [[nodiscard]] std::string getFormattedName(bool identifier) const;

    std::vector<OrganismComponent *> organisms{};

    int age{};

    std::string name{};
    std::string genus{};
    std::string epithet{};

    Genes genes{};
    int32_t generation{};

private:
    size_t id{};
};
