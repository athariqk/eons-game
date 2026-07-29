#include <ncore/game_world.h>
#include <ncore/modules/io/resource_manager.h>
#include <ncore/modules/module_registry.h>
#include <ncore/modules/video/render_module.h>
#include <ncore/modules/video/window_module.h>
#include <ncore/runtime/components/ecs_time.h>
#include <ncore/runtime/components/ecs_transform.h>
#include <ncore/runtime/ecs_base_features.h>
#include <ncore/runtime/ecs_system.h>
#include <ncore/runtime/ecs_world.h>

#include "audio/ecs_audio.h"
#include "debug/ecs_debug.h"
#include "input/ecs_inputs.h"
#include "physics/ecs_physics.h"
#include "video/ecs_gui_feature.h"
#include "video/ecs_render_feature.h"
#include "video/ecs_window_feature.h"

namespace nc {

void EcsBaseFeatures::build( EcsWorld& world )
{
    world.set_singleton<AppDesc>( app_desc );

    world.emplace_singleton<EcsTime>();
    world.emplace_singleton<IoModules>();
    world.emplace_singleton<GraphicsModules>();

    world.create_system( "EcsBaseFeatures::Init" ).in( EcsSystemPhase::INIT ).run( []( QueryContext& ctx ) {
        auto gfx      = ctx.world().get_singleton<GraphicsModules>();
        gfx->window   = ctx.modules().resolve<WindowModule>();
        gfx->renderer = ctx.modules().resolve<RenderModule>();

        auto io       = ctx.world().get_singleton<IoModules>();
        io->resources = ctx.modules().resolve<ResourceManager>();
    } );

    world.create_system( "EcsBaseFeatures::TrackFPS" )
        .with<EcsTime>()
        .in( EcsSystemPhase::PRE_FRAME )
        .run( []( QueryContext& ctx ) {
            auto time = ctx.get_component<EcsTime>();
            time->ticks++;
            time->frame_count++;
            time->accumulator += ctx.delta_time();
            if (time->accumulator >= 1.0) {
                time->fps         = static_cast<double>( time->frame_count ) / time->accumulator;
                time->frame_count = 0;
                time->accumulator = 0.0;
            }
        } );

    world.load_feature<EcsAudioFeature>();
    world.load_feature<EcsInputsFeature>();
    world.load_feature<EcsWindowFeature>();
    world.load_feature<EcsPhysicsFeature>();
    world.load_feature<EcsGuiFeature>();
    world.load_feature<EcsRenderFeature>();

#ifdef NC_DEBUG
    world.load_feature<EcsDebugFeature>();
#endif

    world.create_system( "EcsBaseFeatures::Transform2DPropagator" )
        .in( EcsSystemPhase::UPDATE )
        .with<EcsTransform2D>()
        .each( []( QueryContext& ctx, EcsEntityId eid ) {
            auto xform = ctx.get_component<EcsTransform2D>();

            NC_LOG_TRACE_C(
                log::ECS, "Entity: {}, Transform: {}", eid,
                rtti::TypeRegistry::to_string( xform, rtti::TypeRegistry::get_type_id<EcsTransform2D>() )
            );
        } );

    world.create_system( "EcsBaseFeatures::Transform3DPropagator" )
        .in( EcsSystemPhase::UPDATE )
        .with<EcsTransform3D>()
        .each( []( QueryContext& ctx, EcsEntityId ) {
            //auto xform = ctx.get_component<EcsTransform3D>();

        } );

    world.finalize_ordering();
}

} // namespace nc
