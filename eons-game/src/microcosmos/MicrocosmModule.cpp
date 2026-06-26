#include "MicrocosmModule.h"

#include <ncore/modules/ecs/ecs_world.h>

#include <microcosmos/SpeciesRegistry.h>
#include <microcosmos/systems/FoodSystem.h>
#include <microcosmos/systems/InitSystem.h>
#include <microcosmos/systems/OrganismAISystem.h>
#include <microcosmos/systems/OrganismSystem.h>
#include <microcosmos/systems/SpeciesGuiSystem.h>
#include <microcosmos/systems/SpeciesSystem.h>

void MicrocosmModule::build( ncore::EcsWorld& world )
{
    // world.set<SpeciesRegistry>();
    // world.add_system<SpeciesGuiSystem>();
    // world.add_system<InitSystem>();
    // world.add_system<SpeciesSystem>();
    // world.add_system<OrganismSystem>();
    // world.add_system<OrganismAISystem>();
    // world.add_system<FoodSystem>();
    NC_LOG_WARN( "microcosmos game module is still a stub" );
}
