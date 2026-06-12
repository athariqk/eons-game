#include "SpeciesSystem.h"

#include <modules/World.h>
#include <modules/utils/Logger.h>

#include <microcosmos/MicrocosmWorld.h>
#include <microcosmos/components/OrganismComponent.h>
#include <microcosmos/components/SpeciesComponent.h>

void SpeciesSystem::on_fixed_update(ncore::World &world, double fixedDelta) {
    MicrocosmWorld &micro_world = static_cast<MicrocosmWorld &>(world);

    for (const auto &entity_ptr: world.get_entities()) {
        if (!entity_ptr->get_is_enabled())
            continue;

        if (entity_ptr->has_component<SpeciesComponent>()) {
            auto &species = entity_ptr->get_component<SpeciesComponent>();

            // Check if species has gone extinct
            if (species.population_count <= 0) {
                micro_world.remove_species(&species);
            }
        }
    }
}
