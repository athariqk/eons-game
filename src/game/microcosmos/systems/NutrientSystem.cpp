#include "NutrientSystem.h"

#include <World.h>
#include <Entity.h>
#include <Random.h>
#include <Services.h>
#include <TransformComponent.h>
#include <Viewport.h>

#include "components/NutrientComponent.h"

void NutrientSystem::OnFixedUpdate(Aeon::World &world, double fixedDelta) {
    auto *viewport = world.GetMainLoop().GetServices().TryGet<Aeon::Viewport2D>();

    for (const auto &entityPtr : world.GetEntities()) {
        if (!entityPtr->IsEnabled()) 
            continue;

        if (entityPtr->HasComponent<NutrientComponent>()) {
            auto &nutrient = entityPtr->GetComponent<NutrientComponent>();

            // Destroy if depleted
            if (nutrient.curEnergy < 0) {
                entityPtr->Destroy();
            }
        }
    }
}
