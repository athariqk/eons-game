#include <runtime/ecs/ecs_engine_module.h>

#include <modules/ecs/ecs_world.h>
#include <runtime/ecs/ecs_audio_system.h>
#include <runtime/ecs/ecs_camera_system.h>
#include <runtime/ecs/ecs_input_system.h>
#include <runtime/ecs/ecs_physics_system.h>

namespace ncore {

void EcsEngineModule::build(EcsWorld &world) {
    world.add_system<EcsAudioSystem>();
    world.add_system<EcsPhysicsSystem>();
    world.add_system<EcsCameraSystem>();
    world.add_system<EcsInputSystem>();
}

} // namespace ncore
