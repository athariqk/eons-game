#include "InitSystem.h"

#include <ncore/runtime/ecs_world.h>
#include <ncore/utils/log.h>

#include <microcosmos/OrganismFactory.h>
#include <microcosmos/SpeciesRegistry.h>
#include <microcosmos/components/SpeciesComponent.h>

// void InitSystem::on_init(nc::EcsWorld &world) {
//     if (has_run_)
//         return;
//     has_run_ = true;
//
//     auto &reg = world.get<SpeciesRegistry>();
//
//     auto entity = world.create_entity();
//     // auto species = world.emplace<SpeciesComponent>(entity, "Primum", "Primus", "specium");
//     // species.entity = &entity;
//     // reg.track_species(species);
//     // OrganismFactory::create(world, reg, &species);
//
//     NC_LOG_INFO("Initial species created: Primum Primus specium");
// }
