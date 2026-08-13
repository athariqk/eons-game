#include "scene_plugins.h"

#include <imgui_internal.h>

#include <backends/imgui/imgui_utils.h>

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
#include <ncore/services/input/input_event.h>
#include <ncore/services/input/input_service.h>
#include <ncore/services/video/render_service.h>
#include <ncore/services/video/window/window_event.h>
#include <ncore/services/video/window_service.h>

namespace nc {

void NCAPI register_core_plugin( Scene& scene )
{
    scene.get_ecs().emplace_singleton<TimeComponent>();

    scene.get_ecs()
        .system( "SceneCorePlugin_FPSTracker" )
        .with<TimeComponent>()
        .in( EcsSystemPhase::PRE_FRAME )
        .run( []( QueryContext& ctx ) {
            auto time = ctx.get_component<TimeComponent>();
            time->Ticks++;
            time->FrameCount++;
            time->Accumulator += ctx.delta_time();
            if (time->Accumulator >= 1.0) {
                time->FPS         = static_cast<double>( time->FrameCount ) / time->Accumulator;
                time->FrameCount  = 0;
                time->Accumulator = 0.0;
            }
        } );

    scene.get_ecs().emplace_singleton<IoServices>();
    scene.get_ecs().emplace_singleton<GraphicsServices>();

    scene.get_ecs()
        .system( "Scene_Init" )
        .with<IoServices, GraphicsServices>()
        .in( EcsSystemPhase::INIT )
        .run( [&scene]( QueryContext& ctx ) {
            auto io       = ctx.world().get_singleton<IoServices>();
            auto gfx      = ctx.world().get_singleton<GraphicsServices>();
            io->Resources = scene.get_app_ctx()->Services.resolve<ResourceService>();
            io->Inputs    = scene.get_app_ctx()->Services.resolve<InputService>();
            gfx->Window   = scene.get_app_ctx()->Services.resolve<WindowService>();
            gfx->Renderer = scene.get_app_ctx()->Services.resolve<RenderService>();
        } );
}

void register_window_plugin( Scene& scene )
{
    scene.get_ecs()
        .system( "SceneWindowPlugin_Init" )
        .with<AppDesc>()
        .with<IoServices>()
        .with<GraphicsServices>()
        .in( EcsSystemPhase::INIT )
        .run( []( QueryContext& ctx ) {
            auto app_desc = ctx.get_component<AppDesc>();
            auto io       = ctx.get_component<IoServices>();
            auto gfx      = ctx.get_component<GraphicsServices>();

            gfx->Window->set_default_icon( io->Resources->load<Image>( "images/window.ico" ) );

            auto window_eid = ctx.world()
                                  .entity( "PrimaryWindow" )
                                  .add<MainWindowTag>()
                                  .add<WindowComponent>( WindowComponent{
                                      .Title          = app_desc->Name,
                                      .Resolution     = Vec2( 1280.0f, 720.0f ),
                                      .Fullscreen     = gfx->Window->get_settings().Fullscreen,
                                      .Visible        = true,
                                      .VSync          = gfx->Renderer->get_settings().VSync,
                                      .PixelsPerMeter = gfx->Window->get_settings().PixelsPerMeter
                                  } )
                                  .build();

            ctx.world()
                .entity()
                .add<SwapChainComponent>( SwapChainComponent{ .vsync = gfx->Renderer->get_settings().VSync } )
                .child_of( window_eid )
                .build();
        } );

    scene.get_ecs()
        .observer( "SceneWindowPlugin_ConfigureWindows" )
        .on<WindowComponent>( EcsCoreEvent::OnSet )
        .each( []( QueryContext& ctx, EcsEntity entity_id ) {
            auto win = ctx.get_component<WindowComponent>();
            auto gfx = ctx.world().get_singleton<GraphicsServices>();

            if (win->SourceId == UINT32_MAX) {
                win->SourceId = gfx->Window->window_create();
                gfx->Window->window_set_fullscreen( win->SourceId, win->Fullscreen );
                gfx->Window->window_set_resolution( win->SourceId, win->Resolution );
                gfx->Window->window_set_centered( win->SourceId );
            }

            gfx->Window->window_set_title( win->SourceId, win->Title );
            gfx->Window->window_set_visible( win->SourceId, win->Visible );
        } );

    scene.get_ecs()
        .observer( "SceneWindowPlugin_ConfigureSwapChains" )
        .with<SwapChainComponent>()
        .with<WindowComponent>()
        .up()
        .event( EcsCoreEvent::OnAdd )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto rd  = ctx.get_component<SwapChainComponent>();
            auto win = ctx.get_component<WindowComponent>();
            auto gfx = ctx.world().get_singleton<GraphicsServices>();

            if (!rd->Source.is_valid()) {
                auto whnd  = gfx->Window->get_native_whnd( win->SourceId );
                rd->Source = gfx->Renderer->swapchain_create( whnd, win->Resolution );
                rd->Size   = win->Resolution;
            }
        } );

    scene.get_ecs()
        .observer( "SceneWindowPlugin_DestroySwapChains" )
        .on<SwapChainComponent>( EcsCoreEvent::OnRemove )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto sc  = ctx.get_component<SwapChainComponent>();
            auto gfx = ctx.world().get_singleton<GraphicsServices>();
            if (sc->Source.is_valid()) {
                gfx->Renderer->swapchain_destroy( sc->Source );
                sc->Source = {};
            }
        } );

    scene.get_ecs()
        .observer( "SceneWindowPlugin_DestroyWindows" )
        .on<WindowComponent>( EcsCoreEvent::OnRemove )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto win = ctx.get_component<WindowComponent>();
            auto gfx = ctx.world().get_singleton<GraphicsServices>();
            gfx->Window->window_pop( win->SourceId );
        } );

    scene.get_ecs()
        .system( "SceneWindowPlugin_PumpEvents" )
        .with<GraphicsServices>()
        .in( EcsSystemPhase::PRE_FRAME )
        .run( []( QueryContext& ctx ) {
            auto gfx = ctx.get_component<GraphicsServices>();
            gfx->Window->pump_events();
        } );

    scene.get_ecs()
        .system( "SceneWindowPlugin_ResizeSwapChains" )
        .with<SwapChainComponent>()
        .with<WindowComponent>()
        .up()
        .in( EcsSystemPhase::PRE_FRAME )
        .order( 5 )
        .each( []( QueryContext& ctx, EcsEntity id ) {
            auto win    = ctx.get_component<WindowComponent>();
            auto sc     = ctx.get_component<SwapChainComponent>();
            auto gfx    = ctx.world().get_singleton<GraphicsServices>();
            auto events = gfx->Window->window_events();

            for (const auto& ev : events) {
                if (auto resize = std::get_if<WindowResizeEvent>( &ev )) {
                    if (resize->window_id == win->SourceId) {
                        sc->Size = Vec2( static_cast<float>( resize->width ), static_cast<float>( resize->height ) );
                        ctx.world().emit_event<SwapChainResizedComponent>( { sc->Size }, id );
                        gfx->Renderer->swapchain_set_size( sc->Source, sc->Size );
                    }
                }
            }
        } );

    scene.get_ecs()
        .system( "SceneWindowPlugin_CloseWindows" )
        .with<WindowComponent>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 100 )
        .each( []( QueryContext& ctx, EcsEntity id ) {
            auto win    = ctx.get_component<WindowComponent>();
            auto gfx    = ctx.world().get_singleton<GraphicsServices>();
            auto events = gfx->Window->window_events();

            for (const auto& ev : events) {
                if (auto close = std::get_if<WindowCloseEvent>( &ev )) {
                    if (close->window_id == win->SourceId) {
                        ctx.world().destroy_entity( id );
                    }
                }
            }
        } );
}

void register_render_plugin( Scene& scene )
{
    scene.get_ecs().emplace_singleton<RenderState>();

    scene.get_ecs()
        .system( "SceneRenderPlugin_Init" )
        .with<RenderState>()
        .in( EcsSystemPhase::INIT )
        .run( []( QueryContext& ctx ) {
            auto state          = ctx.get_component<RenderState>();
            auto gfx            = ctx.world().get_singleton<GraphicsServices>();
            uint8_t pixels[4]   = { 255, 255, 255, 255 };
            state->WhiteTexture = gfx->Renderer->texture_2d_create( Image( 1, 1, pixels ) );
        } );

    scene.get_ecs()
        .system( "SceneRenderPlugin_PrepareFrame" )
        .with<SwapChainComponent>()
        .in( EcsSystemPhase::PRE_FRAME )
        .order( 10 )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto sc  = ctx.get_component<SwapChainComponent>();
            auto gfx = ctx.world().get_singleton<GraphicsServices>();
            auto rs  = ctx.world().get_singleton<RenderState>();
            gfx->Renderer->frame_begin();
            rs->DisplaySize = sc->Size;
        } );

    scene.get_ecs()
        .system( "SceneRenderPlugin_Update3DCamera" )
        .with<ActiveCameraTag, CameraComponent, Transform3DComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto gfx   = ctx.world().get_singleton<GraphicsServices>();
            auto cam   = ctx.get_component<CameraComponent>();
            auto xform = ctx.get_component<Transform3DComponent>();
            gfx->Renderer->world_camera_set_fov( cam->FieldOfView );
            gfx->Renderer->world_camera_set_z_far( cam->zFar );
            gfx->Renderer->world_camera_set_z_near( cam->zNear );
            gfx->Renderer->world_camera_set_transform( xform->Global );
        } );

    scene.get_ecs()
        .observer( "SceneRenderPlugin_MaterialInstanceIniter" )
        .with<MaterialComponent>()
        .event<ResourceLoadedComponent>()
        .each( []( QueryContext& ctx, EcsEntity id ) {
            auto state  = ctx.world().get_singleton<RenderState>();
            auto gfx    = ctx.world().get_singleton<GraphicsServices>();
            auto io     = ctx.world().get_singleton<IoServices>();
            auto mat    = ctx.get_component<MaterialComponent>();
            auto loaded = ctx.event_payload<ResourceLoadedComponent>();

            if (mat->Source != loaded->ResourceId)
                return;

            if (mat->Instance)
                gfx->Renderer->destroy_rid( mat->Instance );

            auto source   = io->Resources->get<MaterialTemplate>( loaded->ResourceId );
            mat->Instance = gfx->Renderer->material_create( *source );

            if (mat->TextureCount <= 0)
                mat->Textures[0] = state->WhiteTexture; // fallback texture.

            gfx->Renderer->material_set_texture( mat->Instance, mat->Textures[0], 0 );
        } );

    scene.get_ecs()
        .observer( "SceneRenderPlugin_MeshInstanceIniter" )
        .with<MeshComponent>()
        .event<ResourceLoadedComponent>()
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto gfx    = ctx.world().get_singleton<GraphicsServices>();
            auto io     = ctx.world().get_singleton<IoServices>();
            auto mesh   = ctx.get_component<MeshComponent>();
            auto loaded = ctx.event_payload<ResourceLoadedComponent>();

            if (mesh->Source != loaded->ResourceId)
                return;

            if (mesh->Instance)
                gfx->Renderer->destroy_rid( mesh->Instance );

            auto source    = io->Resources->get<Mesh>( loaded->ResourceId );
            mesh->Instance = gfx->Renderer->gpu_mesh_create( *source );
        } );

    scene.get_ecs()
        .system( "SceneRenderPlugin_MeshInstanceDrawer" )
        .with<MeshComponent>()
        .with<MaterialComponent>()
        .with<Transform3DComponent>()
        .up()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto mesh     = ctx.get_component<MeshComponent>();
            auto material = ctx.get_component<MaterialComponent>();
            auto xform    = ctx.get_component<Transform3DComponent>();
            auto gfx      = ctx.world().get_singleton<GraphicsServices>();

            if (mesh->Instance && material->Instance) {
                gfx->Renderer->world_draw_instance(
                    mesh->Instance, xform->Global, material->Instance, mesh->InstanceCount
                );
            }
        } );

    scene.get_ecs()
        .system( "SceneRenderPlugin_SpriteInstanceDrawer" )
        .with<Transform2DComponent>()
        .with<MaterialComponent>()
        .with<SpriteComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto xform    = ctx.get_component<Transform2DComponent>();
            auto material = ctx.get_component<MaterialComponent>();
            auto sprite   = ctx.get_component<SpriteComponent>();
            auto gfx      = ctx.world().get_singleton<GraphicsServices>();

            float r = math::deg_to_rad( xform->Angle );

            // clang-format off
				auto c_local = xform->Size * 0.5f;
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

            if (material->Instance)
                gfx->Renderer->canvas_draw_quad( world_coords, material->Instance, sprite->Tint );
        } );

    scene.get_ecs()
        .system( "SceneRenderPlugin_EndFrame" )
        .with<SwapChainComponent>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 10 )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto gfx = ctx.world().get_singleton<GraphicsServices>();
            gfx->Renderer->frame_end( static_cast<float>( ctx.delta_time() ) );
        } );
}

void register_inputs_plugin( Scene& scene )
{
    scene.get_ecs()
        .system( "SceneInputPlugin_Init" )
        .with<IoServices>()
        .in( EcsSystemPhase::INIT )
        .run( []( QueryContext& ctx ) {
            auto io = ctx.get_component<IoServices>();
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
        .with<IoServices>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .run( []( QueryContext& ctx ) {
            auto io = ctx.get_component<IoServices>();
            io->Inputs->update();
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_InputComponent_KeyController" )
        .with<InputComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto io    = ctx.world().get_singleton<IoServices>();
            auto input = ctx.get_component<InputComponent>();

            input->Direction.zero();

            if (ImGui::GetIO().WantCaptureKeyboard)
                return;

            Vec2 xz = io->Inputs->action_get_vector(
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
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto io    = ctx.world().get_singleton<IoServices>();
            auto gfx   = ctx.world().get_singleton<GraphicsServices>();
            auto input = ctx.get_component<InputComponent>();

            auto win_id = gfx->Window->get_main_window_id();

            // per-frame deltas, zeroed when the mouse is unlocked
            if (gfx->Window->window_get_mouse_locked( win_id )) {
                auto md               = io->Inputs->get_mouse_delta();
                auto dt               = static_cast<float>( ctx.delta_time() );
                float inv_dt          = ( dt > 0.0f ) ? ( 1.0f / dt ) : 0.0f;
                input->AngularDelta.x = -md.x * inv_dt * input->MouseSensitivity;
                input->AngularDelta.y = -md.y * inv_dt * input->MouseSensitivity;
            } else {
                input->AngularDelta.x = 0;
                input->AngularDelta.y = 0;
            }
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_FlyCamUpdater" )
        .with<ActiveCameraTag, Transform3DComponent, CameraComponent, InputComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto xform = ctx.get_component<Transform3DComponent>();
            auto input = ctx.get_component<InputComponent>();

            auto dt = static_cast<float>( ctx.delta_time() );
            xform->Translation += xform->Rotation * input->Direction * input->Magnitude * dt;

            // 6DOF camera rotation.
            // NOTE: suffers from the so called "holonomy" where if you
            // try to yaw-pitch in a circular manner, then the camera
            // gets tilted ever so slightly
            // Quaternion yaw( input->angular_delta.x * dt, Vec3::up() );
            // Quaternion pitch( input->angular_delta.y * dt, Vec3::right() );
            // Quaternion roll( input->angular_delta.z * dt, Vec3::forward() );
            // xform->Rotation          = xform->Rotation * roll * yaw * pitch;

            // i can't get the above working correctly without unwanted roll
            // so have the one below for now...

            const float yaw_amount   = input->AngularDelta.x * dt;
            const float pitch_amount = input->AngularDelta.y * dt;
            const float roll_amount  = input->AngularDelta.z * dt;

            // FPS-style cam

            // yaw around *world* up
            Quaternion yaw( yaw_amount, Vec3::up() );
            xform->Rotation = yaw * xform->Rotation;

            // pitch around *local* right
            Quaternion pitch( pitch_amount, Vec3::right() );
            xform->Rotation = xform->Rotation * pitch;

            // this roll is useless as it is ignored by the world-up yaw,
            // need to find another solution
            if (!math::is_equal_approx( roll_amount, 0 )) {
                Quaternion roll( roll_amount, Vec3::forward() );
                xform->Rotation = xform->Rotation * roll;
            }

            // good practice
            xform->Rotation = Quaternion::normalize( xform->Rotation );
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_MouseVisibilityHandler" )
        .with<ActiveCameraTag, CameraComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto cam = ctx.get_component<CameraComponent>();
            auto gfx = ctx.world().get_singleton<GraphicsServices>();
            auto io  = ctx.world().get_singleton<IoServices>();

            auto win_id = gfx->Window->get_main_window_id();

            auto rmb_clicked =
                !ImGui::GetIO().WantCaptureMouse && io->Inputs->is_mouse_button_pressed( ButtonIndex::RIGHT );
            if (io->Inputs->is_key_pressed( Key::ESC )) {
                cam->MouseCaptured = false;
            } else if (rmb_clicked) {
                cam->MouseCaptured = !cam->MouseCaptured;
            }
            gfx->Window->window_set_mouse_locked( win_id, cam->MouseCaptured );

            if (cam->MouseCaptured) {
                // confine to center each frame
                gfx->Window->window_set_mouse_position( win_id, gfx->Window->window_get_resolution( win_id ) / 2 );
            }
        } );
}

void register_gui_plugin( Scene& scene )
{
    scene.get_ecs().emplace_singleton<GuiStateComponent>();

    scene.get_ecs()
        .system( "SceneGuiPlugin_Init" )
        .with<GuiStateComponent>()
        .in( EcsSystemPhase::INIT )
        .order( 10 )
        .run( []( QueryContext& ctx ) {
            auto io    = ctx.world().get_singleton<IoServices>();
            auto gfx   = ctx.world().get_singleton<GraphicsServices>();
            auto state = ctx.get_component<GuiStateComponent>();

            IMGUI_CHECKVERSION();
            state->ImGuiCtx   = ImGui::CreateContext();
            ImGuiIO& imgui_io = ImGui::GetIO();
            imgui_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            imgui_io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_RendererHasVtxOffset |
                                     ImGuiBackendFlags_RendererHasTextures;
            imgui_io.Fonts->AddFontFromFileTTF( "assets/fonts/SpaceGrotesk-SemiBold.ttf" );
            imgui_io.Fonts->AddFontFromFileTTF( "assets/fonts/SpaceGrotesk-Regular.ttf" );
            imgui_io.FontDefault = imgui_io.Fonts->AddFontFromFileTTF( "assets/fonts/SpaceGrotesk-Medium.ttf" );

            for (int i = 0; i < ImGuiMouseCursor_COUNT; i++) {
                auto imgui_cursor              = static_cast<ImGuiMouseCursor>( i );
                auto cursor_type               = cursor_type_to_imgui_cursor( imgui_cursor );
                state->CursorMap[imgui_cursor] = cursor_type;
            }

            ImGui::SetCurrentContext( state->ImGuiCtx );

            auto tmpl_rid = io->Resources->load( "materials/canvas.material" );
            auto tmpl     = io->Resources->get<MaterialTemplate>( tmpl_rid );
            NC_VERIFY( tmpl );
            auto mat = gfx->Renderer->material_create( *tmpl );

            state->Material = mat; // TODO: why are we even storing the mat in the global state when
                                   // we're creating a dedicated entity material down below?

            // ctx.world()
            //     .entity( "ImGui_Material" )
            //     .with<MaterialComponent>( { tmpl_rid, mat, {} } )
            //     .child_of( state_id )
            //     .build();
        } );

    scene.get_ecs()
        .observer( "SceneGuiPlugin_Cleanup" )
        .on<GuiStateComponent>( EcsCoreEvent::OnRemove )
        .run( []( QueryContext& ctx ) {
            auto state          = ctx.get_component<GuiStateComponent>();
            auto gfx            = ctx.world().get_singleton<GraphicsServices>();
            ImGuiPlatformIO& io = ImGui::GetPlatformIO();
            for (ImTextureData* tex : io.Textures) {
                if (tex->BackendUserData) {
                    RID rid( static_cast<uint64_t>( reinterpret_cast<uintptr_t>( tex->BackendUserData ) ) );
                    gfx->Renderer->destroy_rid( rid );
                }
                tex->BackendUserData = nullptr;
                tex->SetTexID( ImTextureID_Invalid );
                tex->SetStatus( ImTextureStatus_Destroyed );
            }

            auto& fonts = ImGui::GetIO().Fonts->Fonts;
            for (auto font : fonts) {
                ImGui::GetIO().Fonts->RemoveFont( font );
            }

            if (state->ImGuiCtx)
                ImGui::DestroyContext( state->ImGuiCtx );
        } );

    scene.get_ecs()
        .system( "SceneGuiPlugin_ProcessEvents" )
        .in( EcsSystemPhase::PRE_FRAME )
        .with<GuiStateComponent>()
        .run( []( QueryContext& ctx ) {
            auto io  = ctx.world().get_singleton<IoServices>();
            auto gfx = ctx.world().get_singleton<GraphicsServices>();

            ImGuiIO& gui_io  = ImGui::GetIO();
            gui_io.DeltaTime = static_cast<float>( ctx.delta_time() );

            for (const auto& ev : io->Inputs->get_events()) {
                std::visit(
                    [&]( auto&& e ) {
                        using T = std::decay_t<decltype( e )>;
                        if constexpr (std::is_same_v<T, MouseMotionEvent>) {
                            gui_io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
                            gui_io.AddMousePosEvent( e.position.x, e.position.y );
                        } else if constexpr (std::is_same_v<T, MouseWheelEvent>) {
                            gui_io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
                            gui_io.AddMouseWheelEvent( e.scroll_x, e.scroll_y );
                        } else if constexpr (std::is_same_v<T, MouseButtonEvent>) {
                            int button = -1;
                            if (e.button == ButtonIndex::LEFT)
                                button = 0;
                            else if (e.button == ButtonIndex::RIGHT)
                                button = 1;
                            else if (e.button == ButtonIndex::MIDDLE)
                                button = 2;
                            if (button != -1) {
                                gui_io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
                                gui_io.AddMouseButtonEvent( button, e.action == ButtonAction::PRESS );
                            }
                        } else if constexpr (std::is_same_v<T, TextInputEvent>) {
                            gui_io.AddInputCharactersUTF8( e.text );
                        } else if constexpr (std::is_same_v<T, KeyEvent>) {
                            ImGuiKey key = key_to_imgui_key( e.key );
                            if (key != ImGuiKey_None) {
                                bool down = e.action == ButtonAction::PRESS;
                                gui_io.AddKeyEvent( key, down );
                                gui_io.SetKeyEventNativeData(
                                    key, static_cast<int>( e.key ), 0, static_cast<int>( e.key )
                                );
                            }
                        }
                    },
                    ev
                );
            }

            for (const auto& ev : gfx->Window->window_events()) {
                if (auto focus = std::get_if<WindowFocusEvent>( &ev )) {
                    gui_io.AddFocusEvent( focus->focused );
                }
            }
        } );

    scene.get_ecs()
        .system( "SceneGuiPlugin_PrepareFrame" )
        .in( EcsSystemPhase::PRE_UPDATE )
        .with<SwapChainComponent>()
        .run( []( QueryContext& ctx ) {
            auto rd = ctx.get_component<SwapChainComponent>();

            Vec2 size = rd->Size;
            if (size.is_zero())
                return;

            ImGuiIO& io      = ImGui::GetIO();
            io.DisplaySize.x = size.x;
            io.DisplaySize.y = size.y;

            ImGui::NewFrame();

            // FIXME: improve this. shouldn't access SDL directly, delegate to InputService/EcsInputFeature
            auto gfx = ctx.world().get_singleton<GraphicsServices>();
            SDL_Window* sdl_window =
                SDL_GetWindowFromID( static_cast<SDL_WindowID>( gfx->Window->get_main_window_id() ) );
            if (sdl_window) {
                if (io.WantTextInput) {
                    SDL_StartTextInput( sdl_window );
                } else {
                    SDL_StopTextInput( sdl_window );
                }
            }
        } );

    scene.get_ecs()
        .system( "SceneGuiPlugin_EndFrame" )
        .in( EcsSystemPhase::POST_UPDATE )
        .with<SwapChainComponent>()
        .run( []( QueryContext& ctx ) {
            auto gfx   = ctx.world().get_singleton<GraphicsServices>();
            auto state = ctx.world().get_singleton<GuiStateComponent>();

            ImGui::Render();

            auto wanted_cursor = state->CursorMap.at( ImGui::GetMouseCursor() );
            gfx->Window->set_cursor_type( wanted_cursor );

            ImDrawData* dd = ImGui::GetDrawData();
            if (!dd || dd->DisplaySize.x <= 0 || dd->DisplaySize.y <= 0 || !state->Material.is_valid()) {
                NC_LOG_TRACE_C(
                    log::GRAPHICS, "SceneGuiPlugin_EndFrame: skip (dd={} disp={:.0f}x{:.0f} mat_valid={})",
                    static_cast<void*>( dd ), dd ? dd->DisplaySize.x : 0, dd ? dd->DisplaySize.y : 0,
                    state->Material.is_valid()
                );
                return;
            }
            NC_LOG_TRACE_C(
                log::GRAPHICS, "SceneGuiPlugin_EndFrame: {} cmd lists, {} total vertices", dd->CmdListsCount,
                dd->TotalVtxCount
            );

            auto handle_tex = [&gfx]( ImTextureData* tex ) {
                switch (tex->Status) {
                    case ImTextureStatus_WantCreate: {
                        Image image( tex->Width, tex->Height, tex->GetPixels() );
                        RID rid = gfx->Renderer->texture_2d_create( image );
                        tex->SetTexID( reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( rid.value ) ) );
                        tex->BackendUserData = reinterpret_cast<void*>( static_cast<uintptr_t>( rid.value ) );
                        tex->SetStatus( ImTextureStatus_OK );
                        break;
                    }
                    case ImTextureStatus_WantDestroy: {
                        RID rid( reinterpret_cast<uintptr_t>( tex->BackendUserData ) );
                        gfx->Renderer->destroy_rid( rid );
                        tex->BackendUserData = nullptr;
                        tex->SetTexID( ImTextureID_Invalid );
                        tex->SetStatus( ImTextureStatus_Destroyed );
                        break;
                    }
                    case ImTextureStatus_WantUpdates: {
                        RID old_rid( reinterpret_cast<uintptr_t>( tex->BackendUserData ) );
                        gfx->Renderer->destroy_rid( old_rid );
                        Image image( tex->Width, tex->Height, tex->GetPixels() );
                        RID new_rid = gfx->Renderer->texture_2d_create( image );
                        tex->SetTexID( reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( new_rid.value ) ) );
                        tex->BackendUserData = reinterpret_cast<void*>( static_cast<uintptr_t>( new_rid.value ) );
                        tex->SetStatus( ImTextureStatus_OK );
                        break;
                    }
                    case ImTextureStatus_OK:
                    case ImTextureStatus_Destroyed:
                        break;
                }
            };

            if (dd->Textures) {
                for (auto tex : *dd->Textures) {
                    handle_tex( tex );
                }
            }

            for (auto cmd_list : dd->CmdLists) {
                NC_LOG_TRACE_C(
                    log::GRAPHICS, "  CmdList: {} cmds, {} vtx, {} idx", cmd_list->CmdBuffer.Size,
                    cmd_list->VtxBuffer.Size, cmd_list->IdxBuffer.Size
                );
                for (auto& cmd : cmd_list->CmdBuffer) {
                    if (cmd.UserCallback) {
                        NC_LOG_TRACE_C( log::GRAPHICS, "    UserCallback cmd" );
                        cmd.UserCallback( cmd_list, &cmd );
                        continue;
                    }
                    if (cmd.ElemCount == 0)
                        continue;

                    Rect clip_rect = Rect(
                        ( cmd.ClipRect.x - dd->DisplayPos.x ) * dd->FramebufferScale.x,
                        ( cmd.ClipRect.y - dd->DisplayPos.y ) * dd->FramebufferScale.y,
                        ( cmd.ClipRect.z - cmd.ClipRect.x ) * dd->FramebufferScale.x,
                        ( cmd.ClipRect.w - cmd.ClipRect.y ) * dd->FramebufferScale.y
                    );
                    clip_rect.x = std::max( clip_rect.x, 0.0f );
                    clip_rect.y = std::max( clip_rect.y, 0.0f );

                    auto vtx        = reinterpret_cast<Vertex2D*>( cmd_list->VtxBuffer.Data + cmd.VtxOffset );
                    auto idx        = reinterpret_cast<uint16_t*>( cmd_list->IdxBuffer.Data + cmd.IdxOffset );
                    auto vert_count = cmd_list->VtxBuffer.Size - cmd.VtxOffset;
                    auto idx_count  = cmd.ElemCount;

                    RID tex_id = reinterpret_cast<uintptr_t>( cmd.GetTexID() );
                    if (tex_id != state->LastTexId) {
                        NC_LOG_DEBUG_C(
                            log::GRAPHICS, "  texture change: {} -> {}", state->LastTexId.value, tex_id.value
                        );
                        gfx->Renderer->material_set_texture( state->Material, tex_id, 0 );
                        state->LastTexId = tex_id;
                    }

                    NC_LOG_TRACE_C(
                        log::GRAPHICS, "  canvas_draw_triangles: {} verts, {} idx, clip={:.0f},{:.0f} {:.0f}x{:.0f}",
                        vert_count, idx_count, clip_rect.x, clip_rect.y, clip_rect.w, clip_rect.h
                    );
                    gfx->Renderer->canvas_draw_triangles(
                        { vtx, vert_count }, { idx, idx_count }, state->Material, clip_rect
                    );
                }
            }
        } );
}

void NCAPI register_resources_plugin( Scene& scene )
{
    scene.get_ecs().emplace_singleton<ResourceWatchState>();

    scene.get_ecs()
        .system( "Scene_ResourceWatcher_Poll" )
        .in( EcsSystemPhase::POST_UPDATE )
        .run( []( QueryContext& ctx ) {
            auto io    = ctx.world().get_singleton<IoServices>();
            auto state = ctx.world().get_singleton<ResourceWatchState>();

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
        .each( []( QueryContext& ctx, EcsEntity id ) {
            auto state = ctx.world().get_singleton<ResourceWatchState>();
            for (auto& entry : state->PendingEvents) {
                if (auto loaded = std::get_if<ResourceService::LoadEvent>( &entry )) { // handle a resource loaded event
                    NC_LOG_DEBUG(
                        "ResourceService::LoadEvent: RID={} ResourceFormatID={}", loaded->Handle.value,
                        loaded->FormatId.to_string()
                    );
                    ctx.world().emit_event<ResourceLoadedComponent>( { loaded->Handle, loaded->FormatId }, id );
                }
            }
        } );
}

void NCAPI register_debug_plugin( Scene& scene )
{
    scene.get_ecs()
        .system( "SceneRenderPlugin_DebugView" )
        .with<TimeComponent>()
        .with<GuiStateComponent>()
        .with<GraphicsServices>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( QueryContext& ctx ) {
            auto time = ctx.get_component<TimeComponent>();
            auto gfx  = ctx.get_component<GraphicsServices>();

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 work_pos               = viewport->WorkPos;
            ImVec2 work_size              = viewport->WorkSize;

            ImGui::SetNextWindowBgAlpha( 0.35f );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

            constexpr float PAD      = 10.0f;
            float overlay_right_edge = work_pos.x + work_size.x;
            ImGui::SetNextWindowPos(
                ImVec2( overlay_right_edge - PAD, work_pos.y + PAD ), ImGuiCond_Always, ImVec2( 1.0f, 0.0f )
            );

            ImGui::SetNextWindowSize( ImVec2( 200, 0 ) );

            if (ImGui::Begin( "##overlay", nullptr, window_flags )) {
                ImGui::Text( "Ticks: %u", time->Ticks );
                ImGui::Text( "FPS: %.3f", time->FPS );
                ImGui::Text( "Frame count: %d", time->FrameCount );

                const auto& stats = gfx->Renderer->get_stats();
                ImGui::Text( "GPU time: %.3f ms", stats.gpu_duration_ms );
                ImGui::Text( "IA Prims: %llu", stats.input_primitives );
                ImGui::Text( "IA Verts: %llu", stats.input_vertices );
                ImGui::Text( "VS Invokes: %llu", stats.vs_invocations );
                ImGui::Text( "PS Invokes: %llu", stats.ps_invocations );
                ImGui::Text( "Clipping Prims: %llu", stats.clipping_primitives );
                ImGui::Text( "Clipping Invokes: %llu", stats.clipping_invocations );
            }
            ImGui::End();

            ImGui::PopStyleVar();
        } );
}

} // namespace nc
