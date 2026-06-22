#include <ncore/runtime/ecs/ecs_runtime.h>

#include <ncore/modules/ecs/ecs_world.h>

#include "audio/ecs_audio.h"
#include "debug/ecs_debug.h"
#include "graphics/ecs_graphics.h"
#include "physics/ecs_physics.h"

namespace ncore {

void EcsRuntime::build(EcsWorld &world) {
    world.load<EcsAudio>();
    world.load<EcsGraphics>();
    world.load<EcsPhysics>();

    // TODO: make it possible to do this:
    // world.system<SomeSystem>()
    //		.with<SomeComponent, AnotherComponent>(Relationship)
    //		.priority(X)
    //		.build();

#ifdef NC_DEBUG
    world.load<EcsDebug>();
#endif
}

} // namespace ncore
