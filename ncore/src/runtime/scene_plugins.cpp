#include "scene_plugins.h"

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
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/components/window.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/ecs/ecs_system.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/input/input_event.h>
#include <ncore/services/input/input_service.h>
#include <ncore/services/io/resource_service.h>
#include <ncore/services/video/render_service.h>
#include <ncore/services/video/window/window_event.h>
#include <ncore/services/video/window_service.h>

namespace nc {

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

            gfx->window->set_default_icon( io->resources->load<Image>( "engine/images/default.ico" ) );

            auto window_eid = ctx.world()
                                  .entity( "PrimaryWindow" )
                                  .with<WindowComponent>( WindowComponent{
                                      .title            = app_desc->Name,
                                      .resolution       = Vec2( 1280.0f, 720.0f ),
                                      .fullscreen       = gfx->window->get_settings().Fullscreen,
                                      .visible          = true,
                                      .vsync            = gfx->renderer->get_settings().VSync,
                                      .pixels_per_meter = gfx->window->get_settings().PixelsPerMeter
                                  } )
                                  .with<MainWindowTag>()
                                  .build();

            ctx.world()
                .entity()
                .with<SwapChainComponent>( SwapChainComponent{ .vsync = gfx->renderer->get_settings().VSync } )
                .child_of( window_eid )
                .build();
        } );

    scene.get_ecs()
        .observer( "SceneWindowPlugin_ConfigureWindows" )
        .on<WindowComponent>( EcsCoreEvent::OnSet )
        .each( []( QueryContext& ctx, EcsEntity entity_id ) {
            auto win = ctx.get_component<WindowComponent>();
            auto gfx = ctx.world().get_singleton<GraphicsServices>();

            if (win->id == UINT32_MAX) {
                win->id = gfx->window->window_create();
                gfx->window->window_set_fullscreen( win->id, win->fullscreen );
                gfx->window->window_set_resolution( win->id, win->resolution );
                gfx->window->window_set_centered( win->id );
            }

            gfx->window->window_set_title( win->id, win->title );
            gfx->window->window_set_visible( win->id, win->visible );
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

            if (!rd->swapchain.is_valid()) {
                auto whnd     = gfx->window->get_native_whnd( win->id );
                rd->swapchain = gfx->renderer->swapchain_create( whnd, win->resolution );
                rd->size      = win->resolution;
            }
        } );

    scene.get_ecs()
        .observer( "SceneWindowPlugin_DestroySwapChains" )
        .on<SwapChainComponent>( EcsCoreEvent::OnRemove )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto rd  = ctx.get_component<SwapChainComponent>();
            auto gfx = ctx.world().get_singleton<GraphicsServices>();
            if (rd->swapchain.is_valid()) {
                gfx->renderer->swapchain_destroy( rd->swapchain );
                rd->swapchain = {};
            }
        } );

    scene.get_ecs()
        .observer( "SceneWindowPlugin_DestroyWindows" )
        .on<WindowComponent>( EcsCoreEvent::OnRemove )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto win = ctx.get_component<WindowComponent>();
            auto gfx = ctx.world().get_singleton<GraphicsServices>();
            gfx->window->window_pop( win->id );
        } );

    scene.get_ecs()
        .system( "SceneWindowPlugin_PumpEvents" )
        .with<GraphicsServices>()
        .in( EcsSystemPhase::PRE_FRAME )
        .run( []( QueryContext& ctx ) {
            auto gfx = ctx.get_component<GraphicsServices>();
            gfx->window->pump_events();
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
            auto events = gfx->window->window_events();

            for (const auto& ev : events) {
                if (auto resize = std::get_if<WindowResizeEvent>( &ev )) {
                    if (resize->window_id == win->id) {
                        sc->size = Vec2( static_cast<float>( resize->width ), static_cast<float>( resize->height ) );
                        ctx.world().emit_event<SwapChainResizedComponent>( { sc->size }, id );
                        gfx->renderer->swapchain_set_size( sc->swapchain, sc->size );
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

void register_render_plugin( Scene& scene )
{
    scene.get_ecs().emplace_singleton<RenderState>();

    scene.get_ecs()
        .system( "SceneRenderPlugin_Init" )
        .with<RenderState>()
        .in( EcsSystemPhase::INIT )
        .run( []( QueryContext& ctx ) {
            auto state           = ctx.get_component<RenderState>();
            auto gfx             = ctx.world().get_singleton<GraphicsServices>();
            uint8_t pixels[4]    = { 255, 255, 255, 255 };
            state->white_texture = gfx->renderer->texture_2d_create( Image( 1, 1, pixels ) );
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
            gfx->renderer->frame_begin();
            rs->display_size = sc->size;
        } );

    scene.get_ecs()
        .system( "SceneRenderPlugin_Update3DCamera" )
        .with<CameraComponent, Transform3DComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto gfx   = ctx.world().get_singleton<GraphicsServices>();
            auto cam   = ctx.get_component<CameraComponent>();
            auto xform = ctx.get_component<Transform3DComponent>();
            gfx->renderer->world_camera_set_fov( cam->fov );
            gfx->renderer->world_camera_set_z_far( cam->z_far );
            gfx->renderer->world_camera_set_z_near( cam->z_near );
            gfx->renderer->world_camera_set_transform( xform->get_matrix() );
        } );

    scene.get_ecs()
        .observer( "SceneRenderPlugin_MaterialInstanceIniter" )
        .with<MaterialComponent>()
        .event<ResourceLoadedComponent>()
        .each( []( QueryContext& ctx, EcsEntity id ) {
            auto state    = ctx.world().get_singleton<RenderState>();
            auto gfx      = ctx.world().get_singleton<GraphicsServices>();
            auto io       = ctx.world().get_singleton<IoServices>();
            auto material = ctx.get_component<MaterialComponent>();
            auto loaded   = ctx.event_payload<ResourceLoadedComponent>();

            if (material->source != loaded->resource_id)
                return;

            if (material->instance)
                gfx->renderer->destroy_rid( material->instance );

            auto res              = io->resources->get<MaterialTemplate>( loaded->resource_id );
            material->instance    = gfx->renderer->material_create( *res );
            material->textures[0] = state->white_texture; // TODO: custom textures
            gfx->renderer->material_set_texture( material->instance, material->textures[0], 0 );
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

            if (mesh->source != loaded->resource_id)
                return;

            if (mesh->instance)
                gfx->renderer->destroy_rid( mesh->instance );

            auto res       = io->resources->get<Mesh>( loaded->resource_id );
            mesh->instance = gfx->renderer->gpu_mesh_create( *res );
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

            if (mesh->instance && material->instance) {
                gfx->renderer->world_draw_instance(
                    mesh->instance, xform->get_matrix(), material->instance, mesh->instance_count
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

    scene.get_ecs()
        .system( "SceneRenderPlugin_EndFrame" )
        .with<SwapChainComponent>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 10 )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto gfx = ctx.world().get_singleton<GraphicsServices>();
            gfx->renderer->frame_end( static_cast<float>( ctx.delta_time() ) );
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
            io->inputs->action_bind_event( InputService::FORWARD_ACTION_NAME, KeyEvent{ .key = Key::W } );
            io->inputs->action_bind_event( InputService::BACKWARD_ACTION_NAME, KeyEvent{ .key = Key::S } );
            io->inputs->action_bind_event( InputService::LEFT_ACTION_NAME, KeyEvent{ .key = Key::A } );
            io->inputs->action_bind_event( InputService::RIGHT_ACTION_NAME, KeyEvent{ .key = Key::D } );
            io->inputs->action_bind_event( InputService::UP_ACTION_NAME, KeyEvent{ .key = Key::SPACE } );
            io->inputs->action_bind_event( InputService::DOWN_ACTION_NAME, KeyEvent{ .key = Key::SHIFT } );

            io->inputs->action_register( "G_LeftRoll" );
            io->inputs->action_register( "G_RightRoll" );
            io->inputs->action_bind_event( "G_LeftRoll", KeyEvent{ .key = Key::Q } );
            io->inputs->action_bind_event( "G_RightRoll", KeyEvent{ .key = Key::E } );
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_Update" )
        .with<IoServices>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .run( []( QueryContext& ctx ) {
            auto io = ctx.get_component<IoServices>();
            io->inputs->update();
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_InputComponent_KeyController" )
        .with<InputComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto io    = ctx.world().get_singleton<IoServices>();
            auto input = ctx.get_component<InputComponent>();

            input->direction.zero();

            if (ImGui::GetIO().WantCaptureKeyboard)
                return;

            Vec2 xz = io->inputs->action_get_vector(
                InputService::LEFT_ACTION_NAME, InputService::RIGHT_ACTION_NAME, InputService::BACKWARD_ACTION_NAME,
                InputService::FORWARD_ACTION_NAME
            );
            float y = io->inputs->action_get_axis( InputService::DOWN_ACTION_NAME, InputService::UP_ACTION_NAME );
            input->direction = Vec3( -xz.x, -y, xz.y ); // (-LeftRight, -UpDown, ForwardBackward)

            float r                = io->inputs->action_get_axis( "G_LeftRoll", "G_RightRoll" );
            input->angular_delta.z = -r * input->roll_rate;
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_InputComponent_MouseController" )
        .with<InputComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto io    = ctx.world().get_singleton<IoServices>();
            auto gfx   = ctx.world().get_singleton<GraphicsServices>();
            auto input = ctx.get_component<InputComponent>();

            auto win_id = gfx->window->get_main_window_id();

            if (io->inputs->is_mouse_button_pressed( ButtonIndex::RIGHT )) {
                auto is_locked = gfx->window->window_get_mouse_locked( win_id );
                if (is_locked) {
                    gfx->window->window_set_mouse_position( win_id, gfx->window->window_get_resolution( win_id ) / 2 );
                }
                gfx->window->window_set_mouse_locked( win_id, !is_locked );
            }

            // per-frame deltas, zeroed when the mouse is unlocked
            if (gfx->window->window_get_mouse_locked( win_id )) {
                auto md                = io->inputs->get_mouse_delta();
                auto dt                = static_cast<float>( ctx.delta_time() );
                float inv_dt           = ( dt > 0.0f ) ? ( 1.0f / dt ) : 0.0f;
                input->angular_delta.x = -md.x * inv_dt * input->mouse_sensitivity;
                input->angular_delta.y = -md.y * inv_dt * input->mouse_sensitivity;
            } else {
                input->angular_delta.x = 0;
                input->angular_delta.y = 0;
            }
        } );

    scene.get_ecs()
        .system( "SceneInputPlugin_FlyCamUpdater" )
        .with<Transform3DComponent, CameraComponent, InputComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto xform = ctx.get_component<Transform3DComponent>();
            auto input = ctx.get_component<InputComponent>();

            auto dt = static_cast<float>( ctx.delta_time() );
            xform->translation += xform->rotation * input->direction * input->magnitude * dt;

            // 6DOF camera rotation.
            // NOTE: suffers from the so called "holonomy" where if you
            // try to yaw-pitch in a circular manner, then the camera
            // gets tilted ever so slightly
            Quaternion yaw( input->angular_delta.x * dt, Vec3::up() );
            Quaternion pitch( input->angular_delta.y * dt, Vec3::right() );
            Quaternion roll( input->angular_delta.z * dt, Vec3::forward() );
            xform->rotation = xform->rotation * roll * yaw * pitch;
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
            state->imctx = ImGui::CreateContext();
            StyleColorsNcoreDark();
            StyleSizesNcoreDark();
            ImGuiIO& imgui_io = ImGui::GetIO();
            imgui_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            imgui_io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_RendererHasVtxOffset |
                                     ImGuiBackendFlags_RendererHasTextures;
            imgui_io.Fonts->AddFontFromFileTTF( "assets/engine/fonts/SpaceGrotesk-SemiBold.ttf" );
            imgui_io.Fonts->AddFontFromFileTTF( "assets/engine/fonts/SpaceGrotesk-Regular.ttf" );
            imgui_io.FontDefault = imgui_io.Fonts->AddFontFromFileTTF( "assets/engine/fonts/SpaceGrotesk-Medium.ttf" );

            for (int i = 0; i < ImGuiMouseCursor_COUNT; i++) {
                auto imgui_cursor               = static_cast<ImGuiMouseCursor>( i );
                auto cursor_type                = cursor_type_to_imgui_cursor( imgui_cursor );
                state->cursor_map[imgui_cursor] = cursor_type;
            }

            ImGui::SetCurrentContext( state->imctx );

            auto tmpl_rid = io->resources->load( "engine/materials/canvas.material" );
            auto tmpl     = io->resources->get<MaterialTemplate>( tmpl_rid );
            NC_VERIFY( tmpl );
            auto mat = gfx->renderer->material_create( *tmpl );

            state->material = mat; // TODO: why are we even storing the mat in the global state when
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
                    gfx->renderer->destroy_rid( rid );
                }
                tex->BackendUserData = nullptr;
                tex->SetTexID( ImTextureID_Invalid );
                tex->SetStatus( ImTextureStatus_Destroyed );
            }

            auto& fonts = ImGui::GetIO().Fonts->Fonts;
            for (auto font : fonts) {
                ImGui::GetIO().Fonts->RemoveFont( font );
            }

            if (state->imctx)
                ImGui::DestroyContext( state->imctx );
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

            for (const auto& ev : io->inputs->get_events()) {
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

            for (const auto& ev : gfx->window->window_events()) {
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

            Vec2 size = rd->size;
            if (size.is_zero())
                return;

            ImGuiIO& io      = ImGui::GetIO();
            io.DisplaySize.x = size.x;
            io.DisplaySize.y = size.y;

            ImGui::NewFrame();

            // FIXME: improve this. shouldn't access SDL directly, delegate to InputService/EcsInputFeature
            auto gfx = ctx.world().get_singleton<GraphicsServices>();
            SDL_Window* sdl_window =
                SDL_GetWindowFromID( static_cast<SDL_WindowID>( gfx->window->get_main_window_id() ) );
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

            auto wanted_cursor = state->cursor_map.at( ImGui::GetMouseCursor() );
            gfx->window->set_cursor_type( wanted_cursor );

            ImDrawData* dd = ImGui::GetDrawData();
            if (!dd || dd->DisplaySize.x <= 0 || dd->DisplaySize.y <= 0 || !state->material.is_valid()) {
                NC_LOG_TRACE_C(
                    log::GRAPHICS, "SceneGuiPlugin_EndFrame: skip (dd={} disp={:.0f}x{:.0f} mat_valid={})",
                    static_cast<void*>( dd ), dd ? dd->DisplaySize.x : 0, dd ? dd->DisplaySize.y : 0,
                    state->material.is_valid()
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
                        RID rid = gfx->renderer->texture_2d_create( image );
                        tex->SetTexID( reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( rid.value ) ) );
                        tex->BackendUserData = reinterpret_cast<void*>( static_cast<uintptr_t>( rid.value ) );
                        tex->SetStatus( ImTextureStatus_OK );
                        break;
                    }
                    case ImTextureStatus_WantDestroy: {
                        RID rid( reinterpret_cast<uintptr_t>( tex->BackendUserData ) );
                        gfx->renderer->destroy_rid( rid );
                        tex->BackendUserData = nullptr;
                        tex->SetTexID( ImTextureID_Invalid );
                        tex->SetStatus( ImTextureStatus_Destroyed );
                        break;
                    }
                    case ImTextureStatus_WantUpdates: {
                        RID old_rid( reinterpret_cast<uintptr_t>( tex->BackendUserData ) );
                        gfx->renderer->destroy_rid( old_rid );
                        Image image( tex->Width, tex->Height, tex->GetPixels() );
                        RID new_rid = gfx->renderer->texture_2d_create( image );
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

                    auto vtx        = reinterpret_cast<Vertex2D*>( cmd_list->VtxBuffer.Data + cmd.VtxOffset );
                    auto idx        = reinterpret_cast<uint16_t*>( cmd_list->IdxBuffer.Data + cmd.IdxOffset );
                    auto vert_count = cmd_list->VtxBuffer.Size - cmd.VtxOffset;
                    auto idx_count  = cmd.ElemCount;

                    RID tex_id = reinterpret_cast<uintptr_t>( cmd.GetTexID() );
                    if (tex_id != state->last_tex_id) {
                        NC_LOG_DEBUG_C(
                            log::GRAPHICS, "  texture change: {} -> {}", state->last_tex_id.value, tex_id.value
                        );
                        gfx->renderer->material_set_texture( state->material, tex_id, 0 );
                        state->last_tex_id = tex_id;
                    }

                    NC_LOG_TRACE_C(
                        log::GRAPHICS, "  canvas_draw_triangles: {} verts, {} idx, clip={:.0f},{:.0f} {:.0f}x{:.0f}",
                        vert_count, idx_count, clip_rect.x, clip_rect.y, clip_rect.w, clip_rect.h
                    );
                    gfx->renderer->canvas_draw_triangles(
                        { vtx, vert_count }, { idx, idx_count }, state->material, clip_rect
                    );
                }
            }
        } );
}

} // namespace nc
