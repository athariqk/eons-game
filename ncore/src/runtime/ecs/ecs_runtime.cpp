#include <ncore/modules/ecs/ecs_world.h>
#include <ncore/runtime/ecs/ecs_runtime.h>

#include "audio/ecs_audio.h"
#include "debug/ecs_debug.h"
#include "graphics/ecs_graphics.h"
#include "kernel/ecs_base_feature.h"
#include "physics/ecs_physics.h"

namespace ncore {

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

} // namespace ncore
