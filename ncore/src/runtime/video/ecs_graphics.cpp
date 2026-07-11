#include "ecs_graphics.h"

#include <ncore/application.h>
#include <ncore/game_world.h>
#include <ncore/modules/module_registry.h>
#include <ncore/modules/video/graphics_module.h>
#include <ncore/modules/video/video_module.h>
#include <ncore/runtime/components/ecs_window.h>
#include <ncore/runtime/ecs_system.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

struct GraphicsModules {
    VideoModule* video  = nullptr;
    GraphicsModule* gfx = nullptr;

    NSTRUCT( GraphicsModules, NC_F( GraphicsModules, video ) NC_F( GraphicsModules, gfx ) )
};

void EcsGraphicsFeature::build( EcsWorld& world )
{
    world.emplace_singleton<GraphicsModules>();

    world.create_system( "EcsGraphicsFeature::Init" )
        .with<AppDesc>()
        .with<GraphicsModules>()
        .in( EcsSystemPhase::Init )
        .run( []( QueryContext& ctx ) {
            auto mods   = ctx.get_component<GraphicsModules>();
            mods->video = ctx.modules().resolve<VideoModule>();
            mods->gfx   = ctx.modules().resolve<GraphicsModule>();

            auto window_eid = ctx.world()
                                  .create_entity( "PrimaryWindow" )
                                  .with<EcsWindow>( EcsWindow{
                                      .resolution       = Vec2( 1280.0f, 720.0f ),
                                      .fullscreen       = mods->video->get_settings().Fullscreen,
                                      .visible          = true,
                                      .vsync            = mods->gfx->get_settings().VSync,
                                      .pixels_per_meter = mods->video->get_settings().PixelsPerMeter
                                  } )
                                  .with<EcsMainWindow>()
                                  .build();

            ctx.world()
                .create_entity()
                .with<EcsTargetSurface>( EcsTargetSurface{ .vsync = mods->gfx->get_settings().VSync } )
                .child_of( window_eid )
                .build();

            ctx.world()
                .create_entity( "SecondaryWindow" )
                .with<EcsWindow>( EcsWindow{ .resolution = Vec2( 300.0f, 400.0f ), .visible = true } )
                .build();
        } );

    world.create_observer( "EcsGraphicsFeature::ConfigureWindows" )
        .on<EcsWindow>( EcsCoreEvent::OnSet )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto win   = ctx.get_component<EcsWindow>();
            auto& mods = ctx.world().get_singleton<GraphicsModules>();
            auto& desc = ctx.world().get_singleton<AppDesc>();

            if (win->window_id == UINT32_MAX) {
                win->window_id = mods.video->create_window();
                mods.video->set_window_fullscreen( win->window_id, win->fullscreen );
                mods.video->set_window_resolution( win->window_id, win->resolution );
                mods.video->set_window_centered( win->window_id );
            }

            mods.video->set_window_title( win->window_id, desc.Name );
            mods.video->set_window_visible( win->window_id, win->visible );
        } );

    world.create_observer( "EcsGraphicsFeature::ConfigureSurfaces" )
        .with<EcsTargetSurface>()
        .with<EcsWindow>()
        .up()
        .event( EcsCoreEvent::OnAdd )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto rd    = ctx.get_component<EcsTargetSurface>();
            auto win   = ctx.get_component<EcsWindow>();
            auto& mods = ctx.world().get_singleton<GraphicsModules>();

            if (!rd->surface.is_valid()) {
                rd->surface =
                    mods.gfx->create_surface( mods.video->get_native_whnd( win->window_id ), win->resolution );
                mods.gfx->set_vsync( rd->surface, win->vsync );
            }
        } );

    world.create_observer( "EcsGraphicsFeature::DestroySurfaces" )
        .on<EcsTargetSurface>( EcsCoreEvent::OnRemove )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto rd    = ctx.get_component<EcsTargetSurface>();
            auto& mods = ctx.world().get_singleton<GraphicsModules>();
            if (rd->surface.is_valid()) {
                mods.gfx->destroy_surface( rd->surface );
                rd->surface = {};
            }
        } );

    world.create_observer( "EcsGraphicsFeature::DestroyWindows" )
        .on<EcsWindow>( EcsCoreEvent::OnRemove )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto win   = ctx.get_component<EcsWindow>();
            auto& mods = ctx.world().get_singleton<GraphicsModules>();
            mods.video->pop_window( win->window_id );
        } );

    world.create_system( "EcsGraphicsFeature::PumpEvents" )
        .with<GraphicsModules>()
        .in( EcsSystemPhase::PreFrame )
        .run( []( QueryContext& ctx ) {
            auto mods = ctx.get_component<GraphicsModules>();
            mods->video->pump_events();
        } );

    world.create_system( "EcsGraphicsFeature::PrepareFrame" )
        .with<EcsTargetSurface>()
        .in( EcsSystemPhase::PreFrame )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto rd    = ctx.get_component<EcsTargetSurface>();
            auto& mods = ctx.world().get_singleton<GraphicsModules>();

            if (rd->surface.is_valid())
                mods.gfx->begin_frame( rd->surface );
        } );

    world.create_system( "EcsGraphicsFeature::EndFrame" )
        .with<EcsTargetSurface>()
        .in( EcsSystemPhase::PostFrame )
        .order( 10 )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto rd    = ctx.get_component<EcsTargetSurface>();
            auto& mods = ctx.world().get_singleton<GraphicsModules>();

            if (rd->surface.is_valid())
                mods.gfx->end_frame( rd->surface );
        } );

    world.create_system( "EcsGraphicsFeature::ProcessWindowEvents" )
        .with<EcsWindow>()
        .in( EcsSystemPhase::PostFrame )
        .order( 100 )
        .each( []( QueryContext& ctx, EcsEntityId id ) {
            auto win   = ctx.get_component<EcsWindow>();
            auto& mods = ctx.world().get_singleton<GraphicsModules>();

            for (const auto& ev : mods.video->window_events()) {
                if (auto close = std::get_if<WindowCloseEvent>( &ev )) {
                    if (close->window_id == win->window_id) {
                        ctx.world().destroy_entity( id );
                    }
                }
            }
        } );
}

} // namespace nc
