#include "environment.h"

#include "nutrient.h"
#include "organism.h"
#include "scene.h"

#include "Logger.h"

void Environment::addSpeciesToEnvironment(const std::string &name, const std::string &genus,
                                          const std::string &epithet) {
    auto &instance(Scene::GetEntityManager().AddEntity());
    instance.AddComponent<Species>(name, genus, epithet);
    speciesInEnvironment.push_back(&instance.GetComponent<Species>());

    auto &species = instance.GetComponent<Species>();

    LOG_INFO("Added species {} to the environment, with following traits:\n speed {}, energy capacity {}, size {}",
             species.getFormattedName(true), species.genes.speed, species.genes.energyCapacity, species.genes.size);

    species.addOrganism();
}

Species *Environment::getSpecies(const Species *species) {
    for (auto &s: speciesInEnvironment) {
        if (s == species)
            return s;
    }

    LOG_ERROR("Species is not found!");
    return nullptr;
}

Species *Environment::getSpecies(std::string name) {
    for (auto &s: speciesInEnvironment) {
        if (s->name == name) {
            return s;
        }
    }

    LOG_ERROR("Not found species with name {}", name);
    return nullptr;
}

std::vector<Species *> &Environment::getAllSpecies() { return speciesInEnvironment; }

void Environment::makeExtinct(Species *species) {
    if (species == nullptr) {
        LOG_ERROR("Tried to make a null species extinct!");
        return;
    }

    const auto it = std::ranges::find(speciesInEnvironment, species);
    if (it == speciesInEnvironment.end()) {
        LOG_ERROR("Species {} is not found!", species->getID());
        return;
    }

    const auto formattedName = species->getFormattedName(false);

    species->entity->Destroy();
    species->clearOrganisms();
    speciesInEnvironment.erase(it);

    LOG_INFO("Species {} has gone extinct!", formattedName);
}

int Environment::getSpeciesIndex(const Species *species) const {
    if (const auto it = std::ranges::find(speciesInEnvironment, species); it != speciesInEnvironment.end()) {
        return static_cast<int>(std::distance(speciesInEnvironment.begin(), it));
    }

    LOG_ERROR("Index of species {} is not found!", species != nullptr ? species->getID() : 0);
    return -1;
}

void Environment::spawnNutrients(int amount) {
    for (int i = 0; i < amount; i++) {
        auto &nutrient(Scene::GetEntityManager().AddEntity());
        nutrient.AddComponent<Nutrient>(30);
        nutrient.AddGroup(Scene::groupLabels::NutrientsGroup);
    }
    LOG_INFO("Spawned {} nutrients to the environment", amount);
}
