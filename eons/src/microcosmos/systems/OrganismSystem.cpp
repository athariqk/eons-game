#include "OrganismSystem.h"

#include <modules/MainLoop.h>
#include <modules/Services.h>
#include <modules/World.h>
#include <modules/ecs/components/TransformComponent.h>
#include <modules/graphics/Viewport.h>

#include <utils/Random.h>

#include <microcosmos/Genes.h>
#include <microcosmos/MicrocosmWorld.h>
#include <microcosmos/components/OrganismAIComponent.h>
#include <microcosmos/components/OrganismComponent.h>
#include <microcosmos/components/SpeciesComponent.h>

void OrganismSystem::on_fixed_update(ncore::World &world, double fixedDelta) {
    float delta = static_cast<float>(fixedDelta);

    // Get viewport for spawn positioning
    auto *viewport = world.get_main_loop().get_services().try_get<ncore::Viewport2D>();

    auto &micro_world = static_cast<MicrocosmWorld &>(world);

    // Iterate through all entities
    for (const auto &entity_ptr: world.get_entities()) {
        if (!entity_ptr->get_is_enabled())
            continue;

        if (!entity_ptr->has_component<OrganismComponent>())
            continue;

        auto &organism = entity_ptr->get_component<OrganismComponent>();

        // Update organism state
        organism.cur_energy -= 0.02f * delta;
        organism.fitness -= 0.005f * delta;

        // Check for death condition
        if (organism.cur_energy <= 0) {
            micro_world.remove_organism(&organism);
            micro_world.get_species_by_id(organism.species_id)->population_count--;
            continue;
        }

        // Clamp values
        if (organism.cur_energy < 0)
            organism.cur_energy = 0;

        if (organism.fitness < 0)
            organism.fitness = 0;

        if (organism.cur_energy > organism.genome.energy_capacity) {
            organism.cur_energy = organism.genome.energy_capacity;
        }

        if (organism.fitness > 100) {
            organism.fitness = 100;
        }
    }
}
