#include "Species.h"

#include <iostream>
#include <utility>

#include <AudioManager.h>
#include <Logger.h>

#include "Microcosm.h"
#include "Organism.h"

Species::Species(std::string name, std::string genus, std::string epithet) :
    name(std::move(name)), genus(std::move(genus)), epithet(std::move(epithet)) {}

void Species::OnUpdate(float delta) {
    if (organisms.empty()) {
        // Get the scene and call makeExtinct
        if (auto *scene = dynamic_cast<MicrocosmScene *>(entity->GetManager().GetScene())) {
            scene->MakeExtinct(this);
        }
    }
}

void Species::addOrganism() {
    auto &instance(entity->GetManager().AddEntity());
    instance.AddComponent<OrganismComponent>(this);
    organisms.push_back(&instance.GetComponent<OrganismComponent>());

    auto &organism = instance.GetComponent<OrganismComponent>();

    LOG_INFO("Added organism of species {}, ID: {} with following "
             "traits:\n energy cap {}, speed {}, size {}, aggressiveness {}",
             getFormattedName(false), organism.getID(), organism.genome.energyCapacity, organism.genome.speed,
             organism.genome.size, organism.genome.aggresiveness);

    entity->GetManager().GetScene()->GetAudioManager()->PlayWAV("assets/pop.wav");
}

void Species::addOrganism(Genes &genes, const bool mutate) {
    auto &instance(entity->GetManager().AddEntity());
    instance.AddComponent<OrganismComponent>(this, genes);
    organisms.push_back(&instance.GetComponent<OrganismComponent>());

    auto &organism = instance.GetComponent<OrganismComponent>();

    // Do random mutations
    if (mutate) {
        if (organism.genome.mutate(5, 1)) {
            LOG_INFO("Mutation occurred on organism {}", organism.getID());
        }
    }

    LOG_INFO("Added organism of species {}, ID: {} with following "
             "traits:\n energy cap {}, speed {}, size {}, aggressiveness {}",
             getFormattedName(false), organism.getID(), organism.genome.energyCapacity, organism.genome.speed,
             organism.genome.size, organism.genome.aggresiveness);
}

void Species::deleteOrganism(OrganismComponent *organism) {
    if (organism == nullptr) {
        LOG_ERROR("Tried to delete a null organism!");
        return;
    }

    const auto it = std::ranges::find(organisms, organism);
    if (it == organisms.end()) {
        LOG_ERROR("Organism {} is not found!", organism->getID());
        return;
    }

    organism->entity->Destroy();
    organisms.erase(it);
}

void Species::clearOrganisms() {
    /* Proceed if not empty */
    if (!organisms.empty()) {
        for (auto &it: organisms)
            it->entity->Destroy();

        organisms.clear();
    }
}

int Species::getOrganismIndex(OrganismComponent *organism) {
    if (const auto it = std::ranges::find(organisms, organism); it != organisms.end()) {
        return static_cast<int>(std::distance(organisms.begin(), it));
    }

    LOG_ERROR("Index of organism {} is not found!", organism != nullptr ? organism->getID() : 0);
    return -1;
}

size_t Species::getPopulationCount() const { return organisms.size(); }

std::string Species::getFormattedName(const bool identifier) const {
    std::string result = genus + " " + epithet;

    if (identifier)
        result += " (" + name + ")";

    return result;
}

size_t Species::getID() const { return id; }
