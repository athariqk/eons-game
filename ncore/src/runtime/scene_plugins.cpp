#include "scene_plugins.h"

#include <ncore/application.h>
#include <ncore/resources/resource.h>
#include <ncore/runtime/components/camera.h>
#include <ncore/runtime/components/input.h>
#include <ncore/runtime/components/material.h>
#include <ncore/runtime/components/mesh.h>
#include <ncore/runtime/components/resource.h>
#include <ncore/runtime/components/services.h>
#include <ncore/runtime/components/sprite.h>
#include <ncore/runtime/components/time.h>
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/components/window.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/ecs/ecs_system.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/input_event.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/video/render_service.h>
#include <ncore/services/video/window/window_event.h>
#include <ncore/services/video/window_service.h>
#include <utils/profiler.h>

namespace nc {

void NCAPI register_core_plugin( Scene& scene )
{
    scene.get_ecs().add_singleton<TimeComponent>();

    scene.get_ecs()
        .system( "SceneCorePlugin_FPSTracker" )
        .with<TimeComponent>()
        .in( EcsSystemPhase::PRE_FRAME )
        .run( []( EcsIterState& it ) {
            auto time = it.get_component<TimeComponent>();
            time->ticks++;
            time->frame_count++;
            time->accumulator += it.delta_time();
            if (time->accumulator >= 1.0) {
                time->fps         = static_cast<double>( time->frame_count ) / time->accumulator;
                time->frame_count = 0;
                time->accumulator = 0.0;
            }
            NC_PLOT( "FPS", time->fps );
        } );

    auto io       = scene.get_ecs().add_singleton<IOServices>();
    io->resources = &scene.get_resource_loader();
    io->inputs    = scene.get_app_ctx()->services.resolve<InputService>();

    auto vid      = scene.get_ecs().add_singleton<VideoServices>();
    vid->window   = scene.get_app_ctx()->services.resolve<WindowService>();
    vid->renderer = scene.get_app_ctx()->services.resolve<RenderService>();
}

void register_video_plugin( Scene& scene )
{
    auto window_svc   = scene.get_app_ctx()->services.resolve<WindowService>();
    auto default_icon = scene.get_resource_loader().load<Image>( "images/window.png" );
    window_svc->set_default_icon( default_icon );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_Window_Init" )
        .on<WindowComponent>( ecs::ON_SET )
        .each( []( EcsIterState& it ) {
            auto win = it.get_component<WindowComponent>();
            auto vid = it.world().get_singleton<VideoServices>();

            if (win->source == UINT32_MAX) {
                auto flags = WindowService::DEFAULT_WINDOW_FLAGS;
                if (win->mode == WindowMode::MAXIMIZED) {
                    flags = static_cast<WindowService::WindowFlag>( flags | WindowService::MAXIMIZED );
                } else if (win->mode == WindowMode::FULLSCREEN) {
                    flags = static_cast<WindowService::WindowFlag>( flags | WindowService::FULLSCREEN );
                }
                win->source = vid->window->window_create( flags );
                vid->window->window_set_resolution( win->source, win->resolution );
                auto native    = vid->window->get_native_handle( win->source );
                win->swapchain = vid->renderer->swapchain_create(
                    // NOTE: Screen/swapchain is exclusively a flat 2D render (no depth).
                    // NOTE: This means for depth-requiring renders, it should always go
                    // to an offscreen buffer with depth/stencil tex format enabled
                    native, win->resolution, gfx::TextureFormat::RGBA8_UNORM_SRGB, gfx::TextureFormat::UNKNOWN
                );
                vid->window->window_set_centered( win->source );
            }

            vid->window->window_set_title( win->source, win->title );
            vid->window->window_set_visible( win->source, win->visible );
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_Window_Destroy" )
        .on<WindowComponent>( ecs::ON_REMOVE )
        .each( []( EcsIterState& it ) {
            auto win = it.get_component<WindowComponent>();
            auto vid = it.world().get_singleton<VideoServices>();
            NC_ASSERT_MSG( vid->window->window_pop( win->source ), "Error happened on window destroy." );
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_PumpEvents" )
        .with<VideoServices>()
        .in( EcsSystemPhase::PRE_FRAME )
        .run( []( EcsIterState& it ) {
            auto vid = it.get_component<VideoServices>();
            vid->window->pump_events();
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_Window_HandleEvents" )
        .with<WindowComponent>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 100 )
        .each( []( EcsIterState& it ) {
            auto win    = it.get_component<WindowComponent>();
            auto vid    = it.world().get_singleton<VideoServices>();
            auto events = vid->window->window_events();

            for (const auto& ev : events) {
                if (auto resize = std::get_if<WindowResizeEvent>( &ev )) {
                    if (resize->window_id == win->source) {
                        Vec2i new_size( resize->width, resize->height );
                        vid->renderer->swapchain_set_size( win->swapchain, new_size );
                        it.world().emit_event<WindowResizedComponent>(
                            WindowResizedComponent{ .new_size = new_size }, it.entity()
                        );
                    }
                } else if (auto close = std::get_if<WindowCloseEvent>( &ev )) {
                    if (close->window_id == win->source) {
                        it.world().destroy_entity( it.entity() );
                    }
                }
            }
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_Material_Init" )
        .with<MaterialComponent>()
        .with<MaterialRenderComponent>()
        .event<ResourceLoadedComponent>()
        .each( []( EcsIterState& it ) {
            auto vid    = it.world().get_singleton<VideoServices>();
            auto io     = it.world().get_singleton<IOServices>();
            auto mat    = it.get_component<MaterialComponent>();
            auto handle = it.get_component<MaterialRenderComponent>();
            auto loaded = it.event_payload<ResourceLoadedComponent>();

            if (mat->shader != loaded->resource_id && mat->shader != loaded->replaced)
                return;
            mat->shader = loaded->resource_id;

            auto source = io->resources->get<MaterialShader>( loaded->resource_id );
            if (!source)
                return;

            if (handle->handle)
                vid->renderer->destroy_rid( handle->handle );

            handle->handle = vid->renderer->material_create( source );

            for (int i = 0; i < mat->texture_count; ++i)
                vid->renderer->material_set_texture( handle->handle, mat->textures[i], i );

            vid->renderer->material_set_draw_mode( handle->handle, mat->draw_mode );

            for (const auto& p : mat->params)
                vid->renderer->material_set_param( handle->handle, p.name, p.data.data(), p.data.size() );
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_MaterialComponent_Update" )
        .on<MaterialComponent>( ecs::ON_SET )
        .with<MaterialRenderComponent>()
        .each( []( EcsIterState& it ) {
            auto vid    = it.world().get_singleton<VideoServices>();
            auto mat    = it.get_component<MaterialComponent>();
            auto handle = it.get_component<MaterialRenderComponent>();
            if (!handle->handle)
                return;
            vid->renderer->material_set_draw_mode( handle->handle, mat->draw_mode );
            for (int i = 0; i < mat->texture_count; ++i)
                vid->renderer->material_set_texture( handle->handle, mat->textures[i], i );
            for (const auto& p : mat->params)
                vid->renderer->material_set_param( handle->handle, p.name, p.data.data(), p.data.size() );
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_MaterialRender_Cleanup" )
        .on<MaterialRenderComponent>( ecs::ON_REMOVE )
        .each( []( EcsIterState& it ) {
            auto handle = it.get_component<MaterialRenderComponent>();
            auto vid    = it.world().get_singleton<VideoServices>();
            if (!handle->handle)
                return;
            vid->renderer->destroy_rid( handle->handle );
            handle->handle = 0;
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_Mesh_Init" )
        .with<MeshComponent>()
        .with<MeshRenderComponent>()
        .event<ResourceLoadedComponent>()
        .each( []( EcsIterState& it ) {
            auto vid    = it.world().get_singleton<VideoServices>();
            auto io     = it.world().get_singleton<IOServices>();
            auto mesh   = it.get_component<MeshComponent>();
            auto render = it.get_component<MeshRenderComponent>();
            auto loaded = it.event_payload<ResourceLoadedComponent>();

            if (mesh->source != loaded->resource_id && mesh->source != loaded->replaced)
                return;
            mesh->source = loaded->resource_id;

            // Re-init (resource reload): release the old item AND its GPU mesh;
            // items never own their mesh implicitly.
            if (render->item) {
                auto old_gpu_mesh = vid->renderer->spatial_item_get_mesh( render->item );
                vid->renderer->destroy_rid( render->item );
                if (old_gpu_mesh)
                    vid->renderer->destroy_rid( old_gpu_mesh );
                render->item = 0;
            }

            auto source   = io->resources->get<Mesh>( loaded->resource_id );
            auto gpu_mesh = vid->renderer->mesh_create( *source );

            render->item = vid->renderer->spatial_item_create();
            vid->renderer->spatial_item_set_mesh( render->item, gpu_mesh );
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_MeshRenderer_Cleanup" )
        .on<MeshRenderComponent>( ecs::ON_REMOVE )
        .each( []( EcsIterState& it ) {
            auto render = it.get_component<MeshRenderComponent>();
            auto vid    = it.world().get_singleton<VideoServices>();
            if (!render->item)
                return;
            auto gpu_mesh = vid->renderer->spatial_item_get_mesh( render->item );
            vid->renderer->destroy_rid( render->item );
            if (gpu_mesh)
                vid->renderer->destroy_rid( gpu_mesh );
            render->item = 0;
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_MeshRenderer_Draw" )
        .with<MeshRenderComponent>()
        .with<MaterialRenderComponent>()
        .with<Transform3DComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto mesh     = it.get_component<MeshRenderComponent>();
            auto material = it.get_component<MaterialRenderComponent>();
            auto xform    = it.get_component<Transform3DComponent>();
            auto vid      = it.world().get_singleton<VideoServices>();

            if (mesh->item && material->handle) {
                vid->renderer->spatial_item_set_transform( mesh->item, xform->global );
                vid->renderer->spatial_item_set_material( mesh->item, material->handle );
                vid->renderer->spatial_item_draw( mesh->item, mesh->layer_index, mesh->instance_count );
            }
        } );

    // TODO: disabled. canvas_draw_quad() no longer exists — this needs a rewrite on top of
    // RenderService::canvas_item_add_quad(), and the canvas item has to be owned per-entity
    // (create once, refill each frame) rather than create/draw per tick. No live users today:
    // the only SpriteComponent reference in the repo is also commented out
    // (eons-game/src/microcosmos/systems/FoodSystem.cpp).
    scene.get_ecs()
        .system( "SceneVideoPlugin_SpriteRenderer_Draw" )
        .with<Transform2DComponent>()
        .with<MaterialComponent>()
        .with<SpriteComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            //        auto xform    = it.get_component<Transform2DComponent>();
            //        auto material = it.get_component<MaterialComponent>();
            //        auto sprite   = it.get_component<SpriteComponent>();
            //        auto vid      = it.world().get_singleton<VideoServices>();

            //        float r = math::deg_to_rad( xform->Angle );

            //        // clang-format off
            // auto c_local = xform->Size * 0.5f;
            // Vec2f local_coords[4] = {
            //	{ -c_local.x, -c_local.y },
            //	{  c_local.x, -c_local.y },
            //	{  c_local.x,  c_local.y },
            //	{ -c_local.x,  c_local.y }
            //};
            //        // clang-format on

            //        float cs = std::cos( r );
            //        float sn = std::sin( r );

            //        auto c_world = xform->get_world_center_point();
            //        Vec2f world_coords[4];
            //        for (int i = 0; i < 4; i++) {
            //            const Vec2f& p    = local_coords[i];
            //            world_coords[i].x = p.x * cs - p.y * sn;
            //            world_coords[i].y = p.x * sn + p.y * cs;
            //            world_coords[i] += c_world;
            //        }

            //        if (material->Instance) {
            //            auto item = vid->renderer->canvas_item_create();
            //            vid->renderer->canvas_item_set_material( item, material->Instance );
            //            vid->renderer->canvas_item_add_quad( item, world_coords, Rect2i(), sprite->Tint );
            //            vid->renderer->canvas_item_draw( item );
            //        }
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_BeginFrame" )
        .with<VideoServices>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 0 )
        .run( []( EcsIterState& it ) {
            auto vid = it.get_component<VideoServices>();
            vid->renderer->prepare_frame( static_cast<float>( it.delta_time() ) );
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_Camera_Init" )
        .on<CameraComponent>( ecs::ON_SET )
        .each( []( EcsIterState& it ) {
            auto cam = it.get_component<CameraComponent>();
            auto vid = it.world().get_singleton<VideoServices>();

            auto primary_swapchain = vid->renderer->swapchain_get_primary();
            if (cam->render_to_screen && primary_swapchain) {
                auto screen_size  = vid->renderer->swapchain_get_size( primary_swapchain );
                cam->display_rect = Rect2i( 0, 0, screen_size.x, screen_size.y );
            }

            if (cam->display_rect.has_area()) {
                if (!cam->render_texture) {
                    cam->render_texture = vid->renderer->texture_render_create(
                        cam->display_rect.size(), gfx::TextureFormat::RGBA8_UNORM_SRGB
                    );
                }
                if (!cam->depth_texture) {
                    cam->depth_texture =
                        vid->renderer->texture_render_create( cam->display_rect.size(), gfx::TextureFormat::D32_FLOAT );
                }
            }

            if (!cam->source) {
                cam->source = vid->renderer->camera_create();
                NC_LOG_DEBUG_C(
                    log::ECS, "NEW camera RID={} render_to_screen={}, display_rect={}", cam->source.value,
                    cam->render_to_screen, cam->display_rect
                );
            }
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_Camera_Update" )
        .with<CameraComponent, Transform3DComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto vid   = it.world().get_singleton<VideoServices>();
            auto cam   = it.get_component<CameraComponent>();
            auto xform = it.get_component<Transform3DComponent>();

            auto primary_swapchain = vid->renderer->swapchain_get_primary();
            if (cam->render_to_screen && primary_swapchain) {
                auto screen_size = vid->renderer->swapchain_get_size( primary_swapchain );
                const Rect2i screen_rect( 0, 0, screen_size.x, screen_size.y );
                if (screen_rect.size() != cam->display_rect.size()) {
                    if (cam->render_texture) {
                        vid->renderer->destroy_rid( cam->render_texture );
                        cam->render_texture = 0;
                    }
                    if (cam->depth_texture) {
                        vid->renderer->destroy_rid( cam->depth_texture );
                        cam->depth_texture = 0;
                    }
                    cam->display_rect = screen_rect;
                    NC_LOG_DEBUG_C(
                        log::ECS, "camera targets recreated for rect={}x{} textures={} Source={}", cam->display_rect.w,
                        cam->display_rect.h, cam->render_texture.value, cam->source.value
                    );
                    if (cam->display_rect.has_area()) {
                        cam->render_texture = vid->renderer->texture_render_create(
                            cam->display_rect.size(), gfx::TextureFormat::RGBA8_UNORM_SRGB
                        );
                        cam->depth_texture = vid->renderer->texture_render_create(
                            cam->display_rect.size(), gfx::TextureFormat::D32_FLOAT
                        );
                    }
                }
            }

            if (cam->source) {
                auto attribs          = vid->renderer->camera_attribs_get( cam->source );
                attribs->transform    = xform->global;
                attribs->fov          = cam->field_of_view;
                attribs->z_far        = cam->z_far;
                attribs->z_near       = cam->z_near;
                attribs->display_size = cam->display_rect.size();
            }
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_Camera_Cleanup" )
        .on<CameraComponent>( ecs::ON_REMOVE )
        .each( []( EcsIterState& it ) {
            auto cam = it.get_component<CameraComponent>();
            auto vid = it.world().get_singleton<VideoServices>();
            if (cam->render_texture)
                vid->renderer->destroy_rid( cam->render_texture );
            if (cam->depth_texture)
                vid->renderer->destroy_rid( cam->depth_texture );
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_Camera_Render" )
        .with<CameraComponent>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 10 )
        .each( []( EcsIterState& it ) {
            auto cam = it.get_component<CameraComponent>();
            auto vid = it.world().get_singleton<VideoServices>();

            RenderService::RenderTargetDesc pass{};
            pass.color_texture = cam->render_texture;
            pass.depth_texture = cam->depth_texture;
            pass.camera        = cam->source;
            pass.rectangle     = cam->display_rect;
            pass.draw_spatial  = true;
            pass.draw_canvas   = cam->draw_canvas;
            vid->renderer->render_target( pass );

            // blit offscreen color to swapchain for final display.
            if (cam->render_to_screen)
                vid->renderer->texture_blit( cam->render_texture );
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_EndFrame" )
        .with<VideoServices>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 20 )
        .run( []( EcsIterState& it ) {
            auto vid = it.get_component<VideoServices>();
            vid->renderer->present();
        } );

    auto display_settings = window_svc->get_settings();
    scene.get_ecs()
        .entity( "MainWindow" )
        .add<MainWindowTag>()
        .add<WindowComponent>( WindowComponent{
            .swapchain        = 0, // will be created by the window init system above.
            .title            = scene.get_app_ctx()->app_desc.name,
            .resolution       = Vec2i( display_settings.size_width, display_settings.size_height ),
            .mode             = display_settings.mode,
            .visible          = true,
            .pixels_per_meter = display_settings.pixels_per_meter
        } )
        .build();
}

void register_inputs_plugin( Scene& scene )
{
    auto inputs_svc = scene.get_app_ctx()->services.resolve<InputService>();

    inputs_svc->action_bind_event( InputService::FORWARD_ACTION_NAME, KeyEvent{ .key = Key::W } );
    inputs_svc->action_bind_event( InputService::BACKWARD_ACTION_NAME, KeyEvent{ .key = Key::S } );
    inputs_svc->action_bind_event( InputService::LEFT_ACTION_NAME, KeyEvent{ .key = Key::A } );
    inputs_svc->action_bind_event( InputService::RIGHT_ACTION_NAME, KeyEvent{ .key = Key::D } );
    inputs_svc->action_bind_event( InputService::UP_ACTION_NAME, KeyEvent{ .key = Key::SPACE } );
    inputs_svc->action_bind_event( InputService::DOWN_ACTION_NAME, KeyEvent{ .key = Key::SHIFT } );

    inputs_svc->action_register( "G_LeftRoll" );
    inputs_svc->action_register( "G_RightRoll" );
    inputs_svc->action_bind_event( "G_LeftRoll", KeyEvent{ .key = Key::Q } );
    inputs_svc->action_bind_event( "G_RightRoll", KeyEvent{ .key = Key::E } );

    scene.get_ecs()
        .system( "SceneInputPlugin_Update" )
        .with<IOServices>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .run( []( EcsIterState& it ) {
            auto io = it.get_component<IOServices>();
            io->inputs->update();
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_InputComponent_KeyController" )
        .with<InputComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( EcsIterState& it ) {
            auto io    = it.world().get_singleton<IOServices>();
            auto input = it.get_component<InputComponent>();

            input->direction.zero();

            if (io->inputs->is_ui_keyboard_captured()) {
                input->angular_delta.z = 0.0f;
                return;
            }

            Vec2f xz = io->inputs->action_get_vector(
                InputService::LEFT_ACTION_NAME, InputService::RIGHT_ACTION_NAME, InputService::BACKWARD_ACTION_NAME,
                InputService::FORWARD_ACTION_NAME
            );
            float y = io->inputs->action_get_axis( InputService::DOWN_ACTION_NAME, InputService::UP_ACTION_NAME );
            input->direction = Vec3f( -xz.x, -y, xz.y ); // (-LeftRight, -UpDown, ForwardBackward)

            float r                = io->inputs->action_get_axis( "G_LeftRoll", "G_RightRoll" );
            input->angular_delta.z = -r * input->roll_rate;
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_InputComponent_MouseController" )
        .with<InputComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( EcsIterState& it ) {
            auto io    = it.world().get_singleton<IOServices>();
            auto vid   = it.world().get_singleton<VideoServices>();
            auto input = it.get_component<InputComponent>();

            auto win_id = vid->window->get_main_window_id();

            if (io->inputs->is_ui_keyboard_captured()) {
                input->angular_delta.x = 0;
                input->angular_delta.y = 0;
                return;
            }

            if (vid->window->window_get_mouse_locked( win_id )) {
                // per-frame deltas
                auto md                = io->inputs->get_mouse_delta();
                auto dt                = static_cast<float>( it.delta_time() );
                float inv_dt           = ( dt > 0.0f ) ? ( 1.0f / dt ) : 0.0f;
                input->angular_delta.x = -md.x * inv_dt * input->mouse_sensitivity;
                input->angular_delta.y = -md.y * inv_dt * input->mouse_sensitivity;
            } else {
                input->angular_delta.x = 0;
                input->angular_delta.y = 0;
            }
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_MouseVisibilityHandler" )
        .with<CameraComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto cam = it.get_component<CameraComponent>();
            auto vid = it.world().get_singleton<VideoServices>();
            auto io  = it.world().get_singleton<IOServices>();

            auto win_id = vid->window->get_main_window_id();

            auto rmb_clicked = io->inputs->is_mouse_button_pressed( ButtonIndex::RIGHT );
            if (io->inputs->is_key_pressed( Key::ESC )) {
                cam->mouse_captured = false;
            } else if (rmb_clicked && !io->inputs->is_ui_mouse_captured()) {
                cam->mouse_captured = !cam->mouse_captured;
            }
            vid->window->window_set_mouse_locked( win_id, cam->mouse_captured );

            if (cam->mouse_captured) {
                // confine to center each frame
                auto win_res = vid->window->window_get_resolution( win_id );
                vid->window->window_set_mouse_position(
                    win_id, Vec2f( static_cast<float>( win_res.x ), static_cast<float>( win_res.y ) ) / 2
                );
            }
        } );
}

void NCAPI register_resources_plugin( Scene& scene )
{
    scene.get_ecs().add_singleton<ResourceWatchState>();

    scene.get_ecs()
        .system( "Scene_ResourceWatcher_Poll" )
        .in( EcsSystemPhase::POST_UPDATE )
        .run( []( EcsIterState& it ) {
            auto io    = it.world().get_singleton<IOServices>();
            auto state = it.world().get_singleton<ResourceWatchState>();

            state->pending_events.clear();
            ResourceLoader::Event e;
            while (io->resources->poll_event( &e )) { // has any resource event occurred?
                state->pending_events.push_back( e );
            }
        } );

    // Resource events are dispatched only to entities that actually reference
    // the loaded resource (MeshComponent.Source / MaterialComponent.Shader).
    // Broadcasting every event to every HasResourceTag entity made frame-1
    // cost O(entities x pending events), which blows up with many entities
    // sharing a resource (per-chunk water meshes).
    // NOTE: any new resource-backed component type must be added here.
    auto mesh_entities = scene.get_ecs()
                             .query( "Scene_ResourceWatcher_MeshEntities" )
                             .with<MeshComponent>()
                             .with<HasResourceTag>()
                             .build();

    auto material_entities = scene.get_ecs()
                                 .query( "Scene_ResourceWatcher_MaterialEntities" )
                                 .with<MaterialComponent>()
                                 .with<HasResourceTag>()
                                 .build();

    scene.get_ecs()
        .system( "Scene_ResourceWatcher_Emit" )
        .in( EcsSystemPhase::POST_UPDATE )
        .run( [mesh_entities, material_entities]( EcsIterState& it ) mutable {
            auto state  = it.world().get_singleton<ResourceWatchState>();
            auto& world = it.world();

            for (auto& entry : state->pending_events) {
                auto loaded = std::get_if<ResourceLoader::LoadEvent>( &entry ); // resource loaded event
                if (!loaded)
                    continue;

                ResourceLoadedComponent payload{
                    .resource_id = loaded->handle, .format_id = loaded->format_id, .replaced = loaded->replaced
                };

                for (auto ctx : mesh_entities.entities()) {
                    if (ctx.get_component<MeshComponent>()->source == loaded->handle)
                        world.emit_event<ResourceLoadedComponent>( payload, ctx.entity() );
                }
                for (auto ctx : material_entities.entities()) {
                    if (ctx.get_component<MaterialComponent>()->shader == loaded->handle)
                        world.emit_event<ResourceLoadedComponent>( payload, ctx.entity() );
                }
            }
        } );
}

void NCAPI register_debug_plugin( Scene& scene )
{
    // TODO: figure out what this should do
}

} // namespace nc
