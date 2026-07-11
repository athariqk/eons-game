#include <ncore/runtime/components/ecs_time.h>
#include <ncore/runtime/components/ecs_transform.h>
#include <ncore/runtime/ecs_base_features.h>
#include <ncore/runtime/ecs_system.h>
#include <ncore/runtime/ecs_world.h>

#include "audio/ecs_audio.h"
#include "debug/ecs_debug.h"
#include "input/ecs_inputs.h"
#include "physics/ecs_physics.h"
#include "video/ecs_graphics.h"
#include "video/ecs_gui.h"

namespace nc {

void EcsBaseFeatures::build( EcsWorld& world )
{
    world.set_singleton<AppDesc>( app_desc );

    world.emplace_singleton<EcsTime>();

    world.create_system( "EcsBaseFeature::Time::TrackFPS" )
        .with<EcsTime>()
        .in( EcsSystemPhase::PreFrame )
        .run( []( QueryContext& ctx ) {
            auto time = ctx.get_component<EcsTime>();
            time->ticks++;
            time->frame_count++;
            time->accumulator += ctx.delta_time();
            if (time->accumulator >= 1.0) {
                time->fps = static_cast<double>( time->frame_count ) / time->accumulator;
                time->frame_count = 0;
                time->accumulator = 0.0;
            }
        } );

    world.load_feature<EcsAudioFeature>();
    world.load_feature<EcsGraphicsFeature>();
    world.load_feature<EcsGuiFeature>();
    world.load_feature<EcsPhysicsFeature>();
    world.load_feature<EcsInputsFeature>();

#ifdef NC_DEBUG
    world.load_feature<EcsDebugFeature>();
#endif

    world.create_system( "EcsBaseFeature::TransformPropagator" )
        .in( EcsSystemPhase::Update )
        .with<EcsTransform>()
        .each( []( QueryContext& ctx, EcsEntityId eid ) {
            auto transform = ctx.get_component<EcsTransform>();
            transform->position += Vec2{ 0.1f, 0.0f };
            NC_LOG_TRACE_C(
                log::ECS, "Entity: {}, Transform: {}", eid,
                rtti::TypeRegistry::to_string( transform, rtti::TypeRegistry::get_type_id<EcsTransform>() )
            );
        } );

    world.finalize_ordering();
}

} // namespace nc
