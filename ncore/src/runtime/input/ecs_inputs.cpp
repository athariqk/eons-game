#include "ecs_inputs.h"

#include <ncore/modules/input/input_module.h>
#include <ncore/modules/module_registry.h>
#include <ncore/runtime/ecs_system.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

void EcsInputsFeature::build( EcsWorld& world )
{
    world.create_system( "EcsInputsFeature::PumpEvents" ).in( EcsSystemPhase::PreUpdate ).run( []( QueryContext& ctx ) {
        ctx.modules().resolve<InputModule>()->pump_events();
    } );
}

} // namespace nc
