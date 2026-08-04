#include "ecs_render_feature.h"

#include <ncore/core/vector.h>
#include <ncore/modules/io/resource_manager.h>
#include <ncore/modules/video/render_module.h>
#include <ncore/resources/image.h>
#include <ncore/resources/material_template.h>
#include <ncore/resources/mesh.h>
#include <ncore/runtime/components/ecs_camera.h>
#include <ncore/runtime/components/ecs_material.h>
#include <ncore/runtime/components/ecs_mesh.h>
#include <ncore/runtime/components/ecs_resource.h>
#include <ncore/runtime/components/ecs_sprite.h>
#include <ncore/runtime/components/ecs_transform.h>
#include <ncore/runtime/components/ecs_window.h>
#include <ncore/runtime/ecs_base_features.h>
#include <ncore/runtime/ecs_system.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

void EcsRenderFeature::build( EcsWorld& world )
{
    world.emplace_singleton<RenderState>();

    world.system( "EcsRenderFeature::Init" )
        .with<RenderState>()
        .in( EcsSystemPhase::INIT )
        .run( []( QueryContext& ctx ) {
            auto state           = ctx.get_component<RenderState>();
            auto gfx             = ctx.world().get_singleton<GraphicsModules>();
            uint8_t pixels[4]    = { 255, 255, 255, 255 };
            state->white_texture = gfx->renderer->texture_2d_create( Image( 1, 1, pixels ) );
        } );

    world.system( "EcsRenderFeature::PrepareFrame" )
        .with<EcsSwapChainRef>()
        .in( EcsSystemPhase::PRE_FRAME )
        .order( 10 )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto sc  = ctx.get_component<EcsSwapChainRef>();
            auto gfx = ctx.world().get_singleton<GraphicsModules>();
            auto rs  = ctx.world().get_singleton<RenderState>();
            gfx->renderer->frame_begin();
            rs->display_size = sc->size;
        } );

    world.system( "EcsRenderFeature::Update3DCamera" )
        .with<EcsCamera, EcsTransform3D>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto gfx   = ctx.world().get_singleton<GraphicsModules>();
            auto cam   = ctx.get_component<EcsCamera>();
            auto xform = ctx.get_component<EcsTransform3D>();
            gfx->renderer->world_camera_set_fov( cam->fov );
            gfx->renderer->world_camera_set_z_far( cam->z_far );
            gfx->renderer->world_camera_set_z_near( cam->z_near );
            gfx->renderer->world_camera_set_transform( xform->get_matrix() );
        } );

    world.observer( "EcsRenderFeature::MaterialInstanceIniter" )
        .with<EcsMaterialInstance>()
        .event<EcsResourceLoaded>()
        .each( []( QueryContext& ctx, EcsEntityId id ) {
            auto state    = ctx.world().get_singleton<RenderState>();
            auto gfx      = ctx.world().get_singleton<GraphicsModules>();
            auto io       = ctx.world().get_singleton<IoModules>();
            auto material = ctx.get_component<EcsMaterialInstance>();
            auto loaded   = ctx.event_payload<EcsResourceLoaded>();

            if (material->source != loaded->resource_id)
                return;

            if (material->instance)
                gfx->renderer->destroy_rid( material->instance );

            auto res              = io->resources->get<MaterialTemplate>( loaded->resource_id );
            material->instance    = gfx->renderer->material_create( *res );
            material->textures[0] = state->white_texture; // TODO: custom textures
            gfx->renderer->material_set_texture( material->instance, material->textures[0], 0 );
        } );

    world.observer( "EcsRenderFeature::MeshInstanceIniter" )
        .with<EcsMeshInstance>()
        .event<EcsResourceLoaded>()
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto gfx    = ctx.world().get_singleton<GraphicsModules>();
            auto io     = ctx.world().get_singleton<IoModules>();
            auto mesh   = ctx.get_component<EcsMeshInstance>();
            auto loaded = ctx.event_payload<EcsResourceLoaded>();

            if (mesh->source != loaded->resource_id)
                return;

            if (mesh->instance)
                gfx->renderer->destroy_rid( mesh->instance );

            auto res       = io->resources->get<Mesh>( loaded->resource_id );
            mesh->instance = gfx->renderer->gpu_mesh_create( *res );
        } );

    world.system( "EcsRenderFeature::MeshInstanceDrawer" )
        .with<EcsMeshInstance>()
        .with<EcsMaterialInstance>()
        .with<EcsTransform3D>()
        .up()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto mesh     = ctx.get_component<EcsMeshInstance>();
            auto material = ctx.get_component<EcsMaterialInstance>();
            auto xform    = ctx.get_component<EcsTransform3D>();
            auto gfx      = ctx.world().get_singleton<GraphicsModules>();

            if (mesh->instance && material->instance) {
                gfx->renderer->world_draw_instance(
                    mesh->instance, xform->get_matrix(), material->instance, mesh->instance_count
                );
            }
        } );

    world.system( "EcsRenderFeature::SpriteInstanceDrawer" )
        .with<EcsTransform2D>()
        .with<EcsMaterialInstance>()
        .with<EcsSpriteInstance>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto xform    = ctx.get_component<EcsTransform2D>();
            auto material = ctx.get_component<EcsMaterialInstance>();
            auto sprite   = ctx.get_component<EcsSpriteInstance>();
            auto gfx      = ctx.world().get_singleton<GraphicsModules>();

            float r = math::deg_to_rad( xform->angle );

            // clang-format off
				auto c_local = xform->size * 0.5f;
				Vec2 local_coords[4] = {
					{ -c_local.x, -c_local.y },
					{  c_local.x, -c_local.y },
					{  c_local.x,  c_local.y },
					{ -c_local.x,  c_local.y }
				};
            // clang-format on

            float cs = std::cos( r );
            float sn = std::sin( r );

            auto c_world = xform->get_world_center_point();
            Vec2 world_coords[4];
            for (int i = 0; i < 4; i++) {
                const Vec2& p     = local_coords[i];
                world_coords[i].x = p.x * cs - p.y * sn;
                world_coords[i].y = p.x * sn + p.y * cs;
                world_coords[i] += c_world;
            }

            if (material->instance)
                gfx->renderer->canvas_draw_quad( world_coords, material->instance, sprite->tint );
        } );

    world.system( "EcsRenderFeature::EndFrame" )
        .with<EcsSwapChainRef>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 10 )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto gfx = ctx.world().get_singleton<GraphicsModules>();
            gfx->renderer->frame_end( static_cast<float>( ctx.delta_time() ) );
        } );
}

} // namespace nc
