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
            time->Ticks++;
            time->FrameCount++;
            time->Accumulator += it.delta_time();
            if (time->Accumulator >= 1.0) {
                time->FPS         = static_cast<double>( time->FrameCount ) / time->Accumulator;
                time->FrameCount  = 0;
                time->Accumulator = 0.0;
            }
        } );

    auto io       = scene.get_ecs().add_singleton<IOServices>();
    io->Resources = &scene.get_resource_loader();
    io->Inputs    = scene.get_app_ctx()->Services.resolve<InputService>();

    auto vid      = scene.get_ecs().add_singleton<VideoServices>();
    vid->Window   = scene.get_app_ctx()->Services.resolve<WindowService>();
    vid->Renderer = scene.get_app_ctx()->Services.resolve<RenderService>();
}

void register_video_plugin( Scene& scene )
{
    scene.get_ecs()
        .system( "SceneVideoPlugin_Init" )
        .with<AppDesc>()
        .with<IOServices>()
        .with<VideoServices>()
        .in( EcsSystemPhase::INIT )
        .run( []( EcsIterState& it ) {
            auto app_desc = it.get_component<AppDesc>();
            auto io       = it.get_component<IOServices>();
            auto vid      = it.get_component<VideoServices>();

            vid->Window->set_default_icon( io->Resources->load<Image>( "images/window.png" ) );

            it.world()
                .entity( "MainWindow" )
                .add<MainWindowTag>()
                .add<WindowComponent>( WindowComponent{
                    .Swapchain      = 0,
                    .Title          = app_desc->Name,
                    .Resolution     = Vec2i( 1280, 720 ),
                    .Mode           = vid->Window->get_settings().Mode,
                    .Visible        = true,
                    .PixelsPerMeter = vid->Window->get_settings().PixelsPerMeter
                } )
                .build();
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_CreateWindow" )
        .on<WindowComponent>( EcsCoreEvent::OnSet )
        .each( []( EcsIterState& it ) {
            auto win = it.get_component<WindowComponent>();
            auto vid = it.world().get_singleton<VideoServices>();

            if (win->Source == UINT32_MAX) {
                auto winflag = WindowService::DEFAULT_WINDOW_FLAGS;
                if (win->Mode == WindowMode::MAXIMIZED) {
                    winflag = static_cast<WindowService::WindowFlag>( winflag | WindowService::MAXIMIZED );
                } else if (win->Mode == WindowMode::FULLSCREEN) {
                    winflag = static_cast<WindowService::WindowFlag>( winflag | WindowService::FULLSCREEN );
                }
                win->Source    = vid->Window->window_create( winflag );
                auto nat_hnd   = vid->Window->get_native_handle( win->Source );
                win->Swapchain = vid->Renderer->swapchain_create(
                    // NOTE: Screen/swapchain is exclusively a flat 2D render (no depth).
                    // NOTE: This means for depth-required renders, it should always go to offscreen buffers
                    // with depth/stencil tex format enabled
                    nat_hnd, win->Resolution, gfx::TextureFormat::RGBA8_UNORM_SRGB, gfx::TextureFormat::UNKNOWN
                );
                vid->Window->window_set_resolution( win->Source, win->Resolution );
                vid->Window->window_set_centered( win->Source );
            }

            vid->Window->window_set_title( win->Source, win->Title );
            vid->Window->window_set_visible( win->Source, win->Visible );
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_DestroyWindow" )
        .on<WindowComponent>( EcsCoreEvent::OnRemove )
        .each( []( EcsIterState& it ) {
            auto win = it.get_component<WindowComponent>();
            auto vid = it.world().get_singleton<VideoServices>();
            NC_ASSERT_MSG(
                vid->Window->window_pop( win->Source ), "Error happened on window destroy (from component removal)"
            );
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_PumpEvents" )
        .with<VideoServices>()
        .in( EcsSystemPhase::PRE_FRAME )
        .run( []( EcsIterState& it ) {
            auto vid = it.get_component<VideoServices>();
            vid->Window->pump_events();
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_HandleWindowEvents" )
        .with<WindowComponent>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 100 )
        .each( []( EcsIterState& it ) {
            auto win    = it.get_component<WindowComponent>();
            auto vid    = it.world().get_singleton<VideoServices>();
            auto events = vid->Window->window_events();

            for (const auto& ev : events) {
                if (auto resize = std::get_if<WindowResizeEvent>( &ev )) {
                    if (resize->window_id == win->Source) {
                        Vec2i new_size( resize->width, resize->height );
                        vid->Renderer->swapchain_set_size( win->Swapchain, new_size );
                        it.world().emit_event<WindowResizedComponent>(
                            WindowResizedComponent{ .NewSize = new_size }, it.entity()
                        );
                    }
                } else if (auto close = std::get_if<WindowCloseEvent>( &ev )) {
                    if (close->window_id == win->Source) {
                        it.world().destroy_entity( it.entity() );
                    }
                }
            }
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_Camera_Register" )
        .on<CameraComponent>( EcsCoreEvent::OnAdd )
        .each( []( EcsIterState& it ) {
            auto cam = it.get_component<CameraComponent>();
            auto vid = it.world().get_singleton<VideoServices>();
            if (!cam->Source) {
                cam->Source = vid->Renderer->camera_create();
                NC_LOG_DEBUG_C( log::ECS, "Created camera, RID={}", cam->Source.value );
            }
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_Camera_UpdateTransform" )
        .with<CameraComponent, Transform3DComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto vid   = it.world().get_singleton<VideoServices>();
            auto cam   = it.get_component<CameraComponent>();
            auto xform = it.get_component<Transform3DComponent>();

            if (cam->Source) {
                auto& attribs     = vid->Renderer->camera_get_attribs( cam->Source );
                attribs.Transform = xform->Global;
            }
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_Camera_DestroyRTs" )
        .on<CameraComponent>( EcsCoreEvent::OnRemove )
        .each( []( EcsIterState& it ) {
            auto cam = it.get_component<CameraComponent>();
            auto vid = it.world().get_singleton<VideoServices>();
            if (cam->RenderToScreen) {
                if (cam->RenderTexture)
                    vid->Renderer->destroy_rid( cam->RenderTexture );
                if (cam->DepthTexture)
                    vid->Renderer->destroy_rid( cam->DepthTexture );
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

            if (mat->Shader.handle != loaded->ResourceId)
                return;

            auto source = io->Resources->get<MaterialShader>( loaded->ResourceId );
            if (!source)
                return;

            if (handle->Handle)
                vid->Renderer->destroy_rid( handle->Handle );

            handle->Handle = vid->Renderer->material_create( source );

            for (int i = 0; i < mat->TextureCount; ++i)
                vid->Renderer->material_set_texture( handle->Handle, mat->Textures[i], i );

            vid->Renderer->material_set_draw_mode( handle->Handle, mat->DrawMode );

            for (const auto& p : mat->Params)
                vid->Renderer->material_set_param( handle->Handle, p.name, p.data.data(), p.data.size() );
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_MaterialRenderer_Draw" )
        .on<MaterialComponent>( EcsCoreEvent::OnSet )
        .with<MaterialRenderComponent>()
        .each( []( EcsIterState& it ) {
            auto vid    = it.world().get_singleton<VideoServices>();
            auto mat    = it.get_component<MaterialComponent>();
            auto handle = it.get_component<MaterialRenderComponent>();
            if (!handle->Handle)
                return;
            vid->Renderer->material_set_draw_mode( handle->Handle, mat->DrawMode );
            for (int i = 0; i < mat->TextureCount; ++i)
                vid->Renderer->material_set_texture( handle->Handle, mat->Textures[i], i );
            for (const auto& p : mat->Params)
                vid->Renderer->material_set_param( handle->Handle, p.name, p.data.data(), p.data.size() );
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

            if (mesh->Source != loaded->ResourceId)
                return;

            if (render->Item)
                vid->Renderer->destroy_rid( render->Item );

            auto source   = io->Resources->get<Mesh>( loaded->ResourceId );
            auto gpu_mesh = vid->Renderer->mesh_create( *source );

            render->Item = vid->Renderer->spatial_item_create();
            vid->Renderer->spatial_item_set_mesh( render->Item, gpu_mesh );
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_MeshRenderer_Draw" )
        .with<MeshRenderComponent>()
        .with<MaterialRenderComponent>()
        .with<Transform3DComponent>()
        .up()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto mesh     = it.get_component<MeshRenderComponent>();
            auto material = it.get_component<MaterialRenderComponent>();
            auto xform    = it.get_component<Transform3DComponent>();
            auto vid      = it.world().get_singleton<VideoServices>();

            if (mesh->Item && material->Handle) {
                vid->Renderer->spatial_item_set_transform( mesh->Item, xform->Global );
                vid->Renderer->spatial_item_set_material( mesh->Item, material->Handle );
                vid->Renderer->spatial_item_draw( mesh->Item, mesh->InstanceCount );
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
            //            auto item = vid->Renderer->canvas_item_create();
            //            vid->Renderer->canvas_item_set_material( item, material->Instance );
            //            vid->Renderer->canvas_item_add_quad( item, world_coords, Rect2i(), sprite->Tint );
            //            vid->Renderer->canvas_item_draw( item );
            //        }
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_BeginFrame" )
        .with<VideoServices>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 0 )
        .run( []( EcsIterState& it ) {
            auto vid = it.get_component<VideoServices>();
            vid->Renderer->prepare_frame( static_cast<float>( it.delta_time() ) );
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_Camera_RenderPass" )
        .with<CameraComponent>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 10 )
        .each( []( EcsIterState& it ) {
            auto cam = it.get_component<CameraComponent>();
            auto vid = it.world().get_singleton<VideoServices>();

            if (cam->Source) {
                auto& attribs       = vid->Renderer->camera_get_attribs( cam->Source );
                attribs.Fov         = cam->FieldOfView;
                attribs.zFar        = cam->zFar;
                attribs.zNear       = cam->zNear;
                attribs.DisplaySize = cam->DisplayRect.size();
            }

            if (cam->RenderToScreen) {
                auto screen_size = vid->Renderer->swapchain_get_size( vid->Renderer->swapchain_get_primary() );

                if (cam->DisplayRect.size() != screen_size) {
                    if (cam->RenderTexture)
                        vid->Renderer->destroy_rid( cam->RenderTexture );
                    if (cam->DepthTexture)
                        vid->Renderer->destroy_rid( cam->DepthTexture );

                    cam->RenderTexture =
                        vid->Renderer->texture_render_create( screen_size, gfx::TextureFormat::RGBA8_UNORM_SRGB );
                    cam->DepthTexture =
                        vid->Renderer->texture_render_create( screen_size, gfx::TextureFormat::D32_FLOAT );
                    cam->DisplayRect = Rect2i( 0, 0, screen_size.x, screen_size.y );
                }
            }

             RenderService::RenderFrameDesc pass{};
             pass.color_texture = cam->RenderTexture;
             pass.depth_texture = cam->DepthTexture;
             pass.camera        = cam->Source;
             pass.rectangle     = cam->DisplayRect;
             pass.draw_spatial  = true;
             pass.draw_canvas   = cam->DrawCanvas;
             vid->Renderer->render_frame( pass );

            // blit offscreen color to swapchain.
             if (cam->RenderToScreen)
                 vid->Renderer->texture_blit( cam->RenderTexture );
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_EndFrame" )
        .with<VideoServices>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 20 )
        .run( []( EcsIterState& it ) {
            auto vid = it.get_component<VideoServices>();
            vid->Renderer->present();
        } );
}

void register_inputs_plugin( Scene& scene )
{
    scene.get_ecs()
        .system( "SceneInputPlugin_Init" )
        .with<IOServices>()
        .in( EcsSystemPhase::INIT )
        .run( []( EcsIterState& it ) {
            auto io = it.get_component<IOServices>();
            io->Inputs->action_bind_event( InputService::FORWARD_ACTION_NAME, KeyEvent{ .key = Key::W } );
            io->Inputs->action_bind_event( InputService::BACKWARD_ACTION_NAME, KeyEvent{ .key = Key::S } );
            io->Inputs->action_bind_event( InputService::LEFT_ACTION_NAME, KeyEvent{ .key = Key::A } );
            io->Inputs->action_bind_event( InputService::RIGHT_ACTION_NAME, KeyEvent{ .key = Key::D } );
            io->Inputs->action_bind_event( InputService::UP_ACTION_NAME, KeyEvent{ .key = Key::SPACE } );
            io->Inputs->action_bind_event( InputService::DOWN_ACTION_NAME, KeyEvent{ .key = Key::SHIFT } );

            io->Inputs->action_register( "G_LeftRoll" );
            io->Inputs->action_register( "G_RightRoll" );
            io->Inputs->action_bind_event( "G_LeftRoll", KeyEvent{ .key = Key::Q } );
            io->Inputs->action_bind_event( "G_RightRoll", KeyEvent{ .key = Key::E } );
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_Update" )
        .with<IOServices>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .run( []( EcsIterState& it ) {
            auto io = it.get_component<IOServices>();
            io->Inputs->update();
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_InputComponent_KeyController" )
        .with<InputComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( EcsIterState& it ) {
            auto io    = it.world().get_singleton<IOServices>();
            auto input = it.get_component<InputComponent>();

            input->Direction.zero();

            Vec2f xz = io->Inputs->action_get_vector(
                InputService::LEFT_ACTION_NAME, InputService::RIGHT_ACTION_NAME, InputService::BACKWARD_ACTION_NAME,
                InputService::FORWARD_ACTION_NAME
            );
            float y = io->Inputs->action_get_axis( InputService::DOWN_ACTION_NAME, InputService::UP_ACTION_NAME );
            input->Direction = Vec3( -xz.x, -y, xz.y ); // (-LeftRight, -UpDown, ForwardBackward)

            float r               = io->Inputs->action_get_axis( "G_LeftRoll", "G_RightRoll" );
            input->AngularDelta.z = -r * input->RollRate;
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_InputComponent_MouseController" )
        .with<InputComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( EcsIterState& it ) {
            auto io    = it.world().get_singleton<IOServices>();
            auto vid   = it.world().get_singleton<VideoServices>();
            auto input = it.get_component<InputComponent>();

            auto win_id = vid->Window->get_main_window_id();

            if (vid->Window->window_get_mouse_locked( win_id )) {
                // per-frame deltas
                auto md               = io->Inputs->get_mouse_delta();
                auto dt               = static_cast<float>( it.delta_time() );
                float inv_dt          = ( dt > 0.0f ) ? ( 1.0f / dt ) : 0.0f;
                input->AngularDelta.x = -md.x * inv_dt * input->MouseSensitivity;
                input->AngularDelta.y = -md.y * inv_dt * input->MouseSensitivity;
            } else {
                input->AngularDelta.x = 0;
                input->AngularDelta.y = 0;
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

            auto win_id = vid->Window->get_main_window_id();

            auto rmb_clicked = io->Inputs->is_mouse_button_pressed( ButtonIndex::RIGHT );
            if (io->Inputs->is_key_pressed( Key::ESC )) {
                cam->MouseCaptured = false;
            } else if (rmb_clicked) {
                cam->MouseCaptured = !cam->MouseCaptured;
            }
            vid->Window->window_set_mouse_locked( win_id, cam->MouseCaptured );

            if (cam->MouseCaptured) {
                // confine to center each frame
                auto win_res = vid->Window->window_get_resolution( win_id );
                vid->Window->window_set_mouse_position(
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

            state->PendingEvents.clear();
            ResourceLoader::Event e;
            while (io->Resources->poll_event( &e )) { // has any resource event occurred?
                state->PendingEvents.push_back( e );
            }
        } );

    scene.get_ecs()
        .system( "Scene_ResourceWatcher_Emit" )
        .with<HasResourceTag>()
        .in( EcsSystemPhase::POST_UPDATE )
        .each( []( EcsIterState& it ) {
            auto state = it.world().get_singleton<ResourceWatchState>();
            for (auto& entry : state->PendingEvents) {
                if (auto loaded = std::get_if<ResourceLoader::LoadEvent>( &entry )) { // handle a resource loaded event
                    NC_LOG_DEBUG(
                        "ResourceLoader::LoadEvent: RID={} ResourceFormatID={}", loaded->Handle.value,
                        loaded->FormatId.to_string()
                    );
                    it.world().emit_event<ResourceLoadedComponent>(
                        ResourceLoadedComponent{ .ResourceId = loaded->Handle, .format_id = loaded->FormatId },
                        it.entity()
                    );
                }
            }
        } );
}

void NCAPI register_debug_plugin( Scene& scene )
{
    // TODO: figure out what this should do
}

} // namespace nc
