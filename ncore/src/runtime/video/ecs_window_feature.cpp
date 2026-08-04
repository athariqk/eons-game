#include "ecs_window_feature.h"

#include <ncore/application.h>
#include <ncore/modules/io/resource_manager.h>
#include <ncore/modules/video/render_module.h>
#include <ncore/modules/video/window/window_event.h>
#include <ncore/modules/video/window_module.h>
#include <ncore/resources/image.h>
#include <ncore/runtime/components/ecs_events.h>
#include <ncore/runtime/components/ecs_window.h>
#include <ncore/runtime/ecs_base_features.h>
#include <ncore/runtime/ecs_system.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

void EcsWindowFeature::build( EcsWorld& world )
{
    world.system( "EcsWindowFeature::Init" )
        .with<AppDesc>()
        .with<IoModules>()
        .with<GraphicsModules>()
        .in( EcsSystemPhase::INIT )
        .run( []( QueryContext& ctx ) {
            auto app_desc = ctx.get_component<AppDesc>();
            auto io       = ctx.get_component<IoModules>();
            auto gfx      = ctx.get_component<GraphicsModules>();

            gfx->window->set_default_icon( io->resources->load<Image>( "engine/images/default.ico" ) );

            auto window_eid = ctx.world()
                                  .entity( "PrimaryWindow" )
                                  .with<EcsWindow>( EcsWindow{
                                      .title            = app_desc->Name,
                                      .resolution       = Vec2( 1280.0f, 720.0f ),
                                      .fullscreen       = gfx->window->get_settings().Fullscreen,
                                      .visible          = true,
                                      .vsync            = gfx->renderer->get_settings().VSync,
                                      .pixels_per_meter = gfx->window->get_settings().PixelsPerMeter
                                  } )
                                  .with<EcsMainWindow>()
                                  .build();

            ctx.world()
                .entity()
                .with<EcsSwapChainRef>( EcsSwapChainRef{ .vsync = gfx->renderer->get_settings().VSync } )
                .child_of( window_eid )
                .build();
        } );

    world.observer( "EcsWindowFeature::ConfigureWindows" )
        .on<EcsWindow>( EcsCoreEvent::OnSet )
        .each( []( QueryContext& ctx, EcsEntityId entity_id ) {
            auto win = ctx.get_component<EcsWindow>();
            auto gfx = ctx.world().get_singleton<GraphicsModules>();

            if (win->id == UINT32_MAX) {
                win->id = gfx->window->window_create();
                gfx->window->window_set_fullscreen( win->id, win->fullscreen );
                gfx->window->window_set_resolution( win->id, win->resolution );
                gfx->window->window_set_centered( win->id );
            }

            gfx->window->window_set_title( win->id, win->title );
            gfx->window->window_set_visible( win->id, win->visible );
        } );

    world.observer( "EcsWindowFeature::ConfigureSwapChains" )
        .with<EcsSwapChainRef>()
        .with<EcsWindow>()
        .up()
        .event( EcsCoreEvent::OnAdd )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto rd  = ctx.get_component<EcsSwapChainRef>();
            auto win = ctx.get_component<EcsWindow>();
            auto gfx = ctx.world().get_singleton<GraphicsModules>();

            if (!rd->swapchain.is_valid()) {
                auto whnd     = gfx->window->get_native_whnd( win->id );
                rd->swapchain = gfx->renderer->swapchain_create( whnd, win->resolution );
                rd->size      = win->resolution;
            }
        } );

    world.observer( "EcsWindowFeature::DestroySwapChains" )
        .on<EcsSwapChainRef>( EcsCoreEvent::OnRemove )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto rd  = ctx.get_component<EcsSwapChainRef>();
            auto gfx = ctx.world().get_singleton<GraphicsModules>();
            if (rd->swapchain.is_valid()) {
                gfx->renderer->swapchain_destroy( rd->swapchain );
                rd->swapchain = {};
            }
        } );

    world.observer( "EcsWindowFeature::DestroyWindows" )
        .on<EcsWindow>( EcsCoreEvent::OnRemove )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto win = ctx.get_component<EcsWindow>();
            auto gfx = ctx.world().get_singleton<GraphicsModules>();
            gfx->window->window_pop( win->id );
        } );

    world.system( "EcsWindowFeature::PumpEvents" )
        .with<GraphicsModules>()
        .in( EcsSystemPhase::PRE_FRAME )
        .run( []( QueryContext& ctx ) {
            auto gfx = ctx.get_component<GraphicsModules>();
            gfx->window->pump_events();
        } );

    world.system( "EcsWindowFeature::ResizeSwapChains" )
        .with<EcsSwapChainRef>()
        .with<EcsWindow>()
        .up()
        .in( EcsSystemPhase::PRE_FRAME )
        .order( 5 )
        .each( []( QueryContext& ctx, EcsEntityId id ) {
            auto win    = ctx.get_component<EcsWindow>();
            auto sc     = ctx.get_component<EcsSwapChainRef>();
            auto gfx    = ctx.world().get_singleton<GraphicsModules>();
            auto events = gfx->window->window_events();

            for (const auto& ev : events) {
                if (auto resize = std::get_if<WindowResizeEvent>( &ev )) {
                    if (resize->window_id == win->id) {
                        sc->size = Vec2( static_cast<float>( resize->width ), static_cast<float>( resize->height ) );
                        ctx.world().emit_event<EcsSwapChainResized>( { sc->size }, id );
                        gfx->renderer->swapchain_set_size( sc->swapchain, sc->size );
                    }
                }
            }
        } );

    world.system( "EcsWindowFeature::CloseWindows" )
        .with<EcsWindow>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 100 )
        .each( []( QueryContext& ctx, EcsEntityId id ) {
            auto win    = ctx.get_component<EcsWindow>();
            auto gfx    = ctx.world().get_singleton<GraphicsModules>();
            auto events = gfx->window->window_events();

            for (const auto& ev : events) {
                if (auto close = std::get_if<WindowCloseEvent>( &ev )) {
                    if (close->window_id == win->id) {
                        ctx.world().destroy_entity( id );
                    }
                }
            }
        } );
}

} // namespace nc
