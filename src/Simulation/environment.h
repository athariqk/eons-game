#pragma once
#include "config.h"
#include "species.h"

#include "nutrient.h"

class Environment {
public:
    Environment() = default;
    ~Environment() {}

    void addSpeciesToEnvironment(const std::string &name, const std::string &genus, const std::string &epithet);

    //! \brief Spawn given amount of food to be randomly
    //! scattered around the environment
    static void spawnNutrients(int amount);

    //! \brief Make this species cease to exist
    void makeExtinct(Species *species);

    std::vector<Species *> &getAllSpecies();

    //! \brief Returns species by object
    Species *getSpecies(const Species *species);

    //! \brief Returns species by its name (identifier)
    Species *getSpecies(std::string name);

    int getSpeciesIndex(const Species *species) const;

private:
    std::vector<Species *> speciesInEnvironment;
};
