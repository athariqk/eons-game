#include "SpeciesSystem.h"

#include <ncore/modules/ecs/ecs_world.h>

#include <microcosmos/SpeciesRegistry.h>
#include <microcosmos/components/SpeciesComponent.h>

void SpeciesSystem::on_fixed_update(ncore::EcsWorld &world, double fixedDelta) {
    //auto &reg = world.context<SpeciesRegistry>();

    //for (const auto &entity: world.get_entities()) {
    //    if (!entity.is_enabled)
    //        continue;

    //    if (world.has<SpeciesComponent>(entity)) {
    //        auto &species = world.get<SpeciesComponent>(entity);

    //        if (species.population_count <= 0) {
    //            world.destroy(const_cast<ncore::EcsEntity &>(entity));
    //            reg.untrack_species(&species);
    //        }
    //    }
    //}
}
