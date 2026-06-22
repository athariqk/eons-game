#include "OrganismSystem.h"

#include <ncore/modules/ecs/ecs_world.h>

#include <microcosmos/SpeciesRegistry.h>
#include <microcosmos/components/OrganismComponent.h>

// void OrganismSystem::on_fixed_update(ncore::EcsWorld &world, double fixedDelta) {
//     float delta = static_cast<float>(fixedDelta);

// auto &reg = world.context<SpeciesRegistry>();

// for (const auto &entity: world.get_entities()) {
//     if (!entity.is_enabled)
//         continue;

//    if (!world.has<OrganismComponent>(entity))
//        continue;

//    auto &organism = world.get<OrganismComponent>(entity);

//    organism.cur_energy -= 0.02f * delta;
//    organism.fitness -= 0.005f * delta;

//    if (organism.cur_energy <= 0) {
//        world.destroy(const_cast<ncore::EcsEntity &>(entity));
//        reg.untrack_organism(&organism);
//        auto *species = reg.get_species_by_id(organism.species_id);
//        if (species)
//            species->population_count--;
//        continue;
//    }

//    if (organism.cur_energy < 0)
//        organism.cur_energy = 0;
//    if (organism.fitness < 0)
//        organism.fitness = 0;
//    if (organism.cur_energy > organism.genome.energy_capacity)
//        organism.cur_energy = organism.genome.energy_capacity;
//    if (organism.fitness > 100)
//        organism.fitness = 100;
//}
//}
