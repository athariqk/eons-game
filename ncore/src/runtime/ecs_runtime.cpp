#include <ncore/runtime/ecs_runtime.h>
#include <ncore/runtime/ecs_world.h>

#include "audio/ecs_audio.h"
#include "debug/ecs_debug.h"
#include "ecs_base_feature.h"
#include "graphics/ecs_graphics.h"
#include "physics/ecs_physics.h"

namespace nc {

void EcsRuntimeFeature::build( EcsWorld& world )
{
    world.load_feature<EcsBaseFeature>();
    world.load_feature<EcsAudioFeature>();
    world.load_feature<EcsGraphicsFeature>();
    world.load_feature<EcsPhysicsFeature>();

#ifdef NC_DEBUG
    world.load_feature<EcsDebugFeature>();
#endif
}

} // namespace nc
