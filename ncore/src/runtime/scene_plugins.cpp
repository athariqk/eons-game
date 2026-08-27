#include "scene_plugins.h"

#include <ncore/application.h>
#include <ncore/resources/image.h>
#include <ncore/resources/material_template.h>
#include <ncore/resources/mesh.h>
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
#include <ncore/services/io/resource_service.h>
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
    io->Resources = scene.get_app_ctx()->Services.resolve<ResourceService>();
    io->Inputs    = scene.get_app_ctx()->Services.resolve<InputService>();

    auto vid    = scene.get_ecs().add_singleton<VideoServices>();
    vid->Window = scene.get_app_ctx()->Services.resolve<WindowService>();
    vid->Gfx    = scene.get_app_ctx()->Services.resolve<RenderService>();
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

            vid->Window->set_default_icon( io->Resources->load<Image>( "images/window.ico" ) );

            it.world()
                .entity( "MainWindow" )
                .add<MainWindowTag>()
                .add<WindowComponent>( WindowComponent{
                    .Swapchain      = 0,
                    .Title          = app_desc->Name,
                    .Resolution     = Vec2i( 1280, 720 ),
                    .Fullscreen     = vid->Window->get_settings().Fullscreen,
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
                win->Source    = vid->Window->window_create();
                auto nat_hnd   = vid->Window->get_native_handle( win->Source );
                win->Swapchain = vid->Gfx->swapchain_create(
                    // NOTE: Screen/swapchain is exclusively a flat 2D render (no depth).
                    // NOTE: This means for depth-required renders, i think it should always go to offscreen buffers
                    // with depth/stencil tex format enabled
                    nat_hnd, win->Resolution, TextureFormat::RGBA8_UNORM_SRGB, TextureFormat::UNKNOWN
                );
                vid->Window->window_set_fullscreen( win->Source, win->Fullscreen );
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
            NC_ASSERT(
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
                        vid->Gfx->swapchain_set_size( win->Swapchain, new_size );
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
                cam->Source = vid->Gfx->camera_create();
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
                auto& attribs     = vid->Gfx->camera_get_attribs( cam->Source );
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
                    vid->Gfx->destroy_rid( cam->RenderTexture );
                if (cam->DepthTexture)
                    vid->Gfx->destroy_rid( cam->DepthTexture );
            }
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_MaterialInstanceIniter" )
        .with<MaterialComponent>()
        .event<ResourceLoadedComponent>()
        .each( []( EcsIterState& it ) {
            auto vid    = it.world().get_singleton<VideoServices>();
            auto io     = it.world().get_singleton<IOServices>();
            auto mat    = it.get_component<MaterialComponent>();
            auto loaded = it.event_payload<ResourceLoadedComponent>();

            if (mat->Source != loaded->ResourceId)
                return;

            if (mat->Instance)
                vid->Gfx->destroy_rid( mat->Instance );

            auto source   = io->Resources->get<MaterialTemplate>( loaded->ResourceId );
            mat->Instance = vid->Gfx->material_create( *source );

            // if no texture exists, renderer will fallback to a missing texture.
            auto tex = mat->TextureCount > 0 ? mat->Textures[0] : RID();
            vid->Gfx->material_set_texture( mat->Instance, tex, 0 );
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_MaterialInstanceUpdater" )
        .on<MaterialComponent>( EcsCoreEvent::OnSet )
        .each( []( EcsIterState& it ) {
            auto vid = it.world().get_singleton<VideoServices>();
            auto mat = it.get_component<MaterialComponent>();
            if (mat->Instance)
                vid->Gfx->material_set_draw_mode( mat->Instance, mat->DrawMode );
        } );

    scene.get_ecs()
        .observer( "SceneVideoPlugin_MeshInstanceIniter" )
        .with<MeshComponent>()
        .event<ResourceLoadedComponent>()
        .each( []( EcsIterState& it ) {
            auto vid    = it.world().get_singleton<VideoServices>();
            auto io     = it.world().get_singleton<IOServices>();
            auto mesh   = it.get_component<MeshComponent>();
            auto loaded = it.event_payload<ResourceLoadedComponent>();

            if (mesh->Source != loaded->ResourceId)
                return;

            if (mesh->Instance)
                vid->Gfx->destroy_rid( mesh->Instance );

            auto source    = io->Resources->get<Mesh>( loaded->ResourceId );
            mesh->Instance = vid->Gfx->gpu_mesh_create( *source );
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_MeshInstanceDrawer" )
        .with<MeshComponent>()
        .with<MaterialComponent>()
        .with<Transform3DComponent>()
        .up()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto mesh     = it.get_component<MeshComponent>();
            auto material = it.get_component<MaterialComponent>();
            auto xform    = it.get_component<Transform3DComponent>();
            auto vid      = it.world().get_singleton<VideoServices>();

            if (mesh->Instance && material->Instance) {
                vid->Gfx->spatial_draw_instance(
                    mesh->Instance, xform->Global, material->Instance, mesh->InstanceCount
                );
            }
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_SpriteInstanceDrawer" )
        .with<Transform2DComponent>()
        .with<MaterialComponent>()
        .with<SpriteComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto xform    = it.get_component<Transform2DComponent>();
            auto material = it.get_component<MaterialComponent>();
            auto sprite   = it.get_component<SpriteComponent>();
            auto vid      = it.world().get_singleton<VideoServices>();

            float r = math::deg_to_rad( xform->Angle );

            // clang-format off
				auto c_local = xform->Size * 0.5f;
				Vec2f local_coords[4] = {
					{ -c_local.x, -c_local.y },
					{  c_local.x, -c_local.y },
					{  c_local.x,  c_local.y },
					{ -c_local.x,  c_local.y }
				};
            // clang-format on

            float cs = std::cos( r );
            float sn = std::sin( r );

            auto c_world = xform->get_world_center_point();
            Vec2f world_coords[4];
            for (int i = 0; i < 4; i++) {
                const Vec2f& p    = local_coords[i];
                world_coords[i].x = p.x * cs - p.y * sn;
                world_coords[i].y = p.x * sn + p.y * cs;
                world_coords[i] += c_world;
            }

            if (material->Instance)
                vid->Gfx->canvas_draw_quad( world_coords, material->Instance, 0, sprite->Tint );
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_BeginFrame" )
        .with<VideoServices>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 0 )
        .run( []( EcsIterState& it ) {
            auto vid = it.get_component<VideoServices>();
            vid->Gfx->render_begin( static_cast<float>( it.delta_time() ) );
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
                auto& attribs       = vid->Gfx->camera_get_attribs( cam->Source );
                attribs.Fov         = cam->FieldOfView;
                attribs.zFar        = cam->zFar;
                attribs.zNear       = cam->zNear;
                attribs.DisplaySize = cam->DisplayRect.size();
            }

            if (cam->RenderToScreen) {
                auto screen_size = vid->Gfx->swapchain_get_size( vid->Gfx->swapchain_get_primary() );

                if (cam->DisplayRect.size() != screen_size) {
                    if (cam->RenderTexture)
                        vid->Gfx->destroy_rid( cam->RenderTexture );
                    if (cam->DepthTexture)
                        vid->Gfx->destroy_rid( cam->DepthTexture );

                    cam->RenderTexture =
                        vid->Gfx->texture_render_create( screen_size, TextureFormat::RGBA8_UNORM_SRGB );
                    cam->DepthTexture = vid->Gfx->texture_render_create( screen_size, TextureFormat::D32_FLOAT );
                    cam->DisplayRect  = Rect2i( 0, 0, screen_size.x, screen_size.y );
                }
            }

            RenderService::RenderPassDesc pass{};
            pass.color_target = cam->RenderTexture;
            pass.depth_target = cam->DepthTexture;
            pass.camera       = cam->Source;
            pass.target_rect  = cam->DisplayRect;
            pass.draw_spatial = true;
            pass.draw_canvas  = cam->DrawCanvas;
            vid->Gfx->render_pass( pass );

            // Blit offscreen color to swapchain.
            if (cam->RenderToScreen)
                vid->Gfx->texture_blit( cam->RenderTexture );
        } );

    scene.get_ecs()
        .system( "SceneVideoPlugin_EndFrame" )
        .with<VideoServices>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 20 )
        .run( []( EcsIterState& it ) {
            auto vid = it.get_component<VideoServices>();
            vid->Gfx->present();
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
            ResourceService::Event e;
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
                if (auto loaded = std::get_if<ResourceService::LoadEvent>( &entry )) { // handle a resource loaded event
                    NC_LOG_DEBUG(
                        "ResourceService::LoadEvent: RID={} ResourceFormatID={}", loaded->Handle.value,
                        loaded->FormatId.to_string()
                    );
                    it.world().emit_event<ResourceLoadedComponent>( { loaded->Handle, loaded->FormatId }, it.entity() );
                }
            }
        } );
}

void NCAPI register_debug_plugin( Scene& scene )
{
    // TODO: figure out what this should do
}

} // namespace nc
