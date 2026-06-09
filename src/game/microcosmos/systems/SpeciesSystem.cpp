#include "SpeciesSystem.h"

#include <Logger.h>
#include <World.h>

#include "MicrocosmWorld.h"
#include "components/OrganismComponent.h"
#include "components/SpeciesComponent.h"

void SpeciesSystem::OnFixedUpdate(Aeon::World &world, double fixedDelta) {
    MicrocosmWorld &microWorld = static_cast<MicrocosmWorld &>(world);

    for (const auto &entityPtr: world.GetEntities()) {
        if (!entityPtr->IsEnabled())
            continue;

        if (entityPtr->HasComponent<SpeciesComponent>()) {
            auto &species = entityPtr->GetComponent<SpeciesComponent>();

            // Check if species has gone extinct
            if (species.populationCount <= 0) {
                microWorld.MakeExtinct(&species);
            }
        }
    }
}
