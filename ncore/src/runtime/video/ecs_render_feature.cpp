#include "ecs_render_feature.h"

#include <ncore/modules/video/render_module.h>
#include <ncore/runtime/components/ecs_window.h>
#include <ncore/runtime/ecs_base_features.h>
#include <ncore/runtime/ecs_system.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

void EcsRenderFeature::build( EcsWorld& world )
{
    world.emplace_singleton<RenderState>();

    world.create_system( "EcsRenderFeature::PrepareFrame" )
        .with<EcsSwapChainRef>()
        .in( EcsSystemPhase::PRE_FRAME )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto sc  = ctx.get_component<EcsSwapChainRef>();
            auto gfx = ctx.world().get_singleton<GraphicsModules>();
            auto rs  = ctx.world().get_singleton<RenderState>();
            gfx->renderer->frame_begin();
            rs->display_size = sc->size;
        } );

    world.create_system( "EcsRenderFeature::EndFrame" )
        .with<EcsSwapChainRef>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 10 )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto gfx = ctx.world().get_singleton<GraphicsModules>();
            gfx->renderer->frame_end();
        } );
}

} // namespace nc
