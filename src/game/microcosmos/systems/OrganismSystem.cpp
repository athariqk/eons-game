#include "OrganismSystem.h"

#include <Logger.h>
#include <Random.h>
#include <Services.h>
#include <TransformComponent.h>
#include <Viewport.h>
#include <World.h>

#include "Genes.h"
#include "MicrocosmWorld.h"
#include "components/OrganismAIComponent.h"
#include "components/OrganismComponent.h"
#include "components/SpeciesComponent.h"

void OrganismSystem::OnFixedUpdate(Aeon::World &world, double fixedDelta) {
    float delta = static_cast<float>(fixedDelta);

    // Get viewport for spawn positioning
    auto *viewport = world.GetMainLoop().GetServices().TryGet<Aeon::Viewport2D>();

    auto &microWorld = static_cast<MicrocosmWorld &>(world);

    // Iterate through all entities
    for (const auto &entityPtr: world.GetEntities()) {
        if (!entityPtr->IsEnabled())
            continue;

        if (!entityPtr->HasComponent<OrganismComponent>())
            continue;

        auto &organism = entityPtr->GetComponent<OrganismComponent>();

        // Update organism state
        organism.curEnergy -= 0.02f * delta;
        organism.fitness -= 0.005f * delta;

        // Check for death condition
        if (organism.curEnergy <= 0) {
            microWorld.DeleteOrganism(&organism);
            microWorld.GetSpeciesById(organism.speciesId)->populationCount--;
            continue;
        }

        // Clamp values
        if (organism.curEnergy < 0)
            organism.curEnergy = 0;

        if (organism.fitness < 0)
            organism.fitness = 0;

        if (organism.curEnergy > organism.genome.energyCapacity) {
            organism.curEnergy = organism.genome.energyCapacity;
        }

        if (organism.fitness > 100) {
            organism.fitness = 100;
        }
    }
}
