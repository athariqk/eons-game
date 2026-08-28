#include "gui_plugin.h"

#include <ncore/resources/material_template.h>
#include <ncore/runtime/components/services.h>
#include <ncore/runtime/components/window.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/io/resource_service.h>
#include <ncore/services/video/render_service.h>
#include <ncore/services/video/window_service.h>

#include "editor_state.h"
#include "imgui_style.h"
#include "imgui_utils.h"

namespace nc::editor {

void register_gui_plugin( Scene& scene )
{
    scene.get_ecs().add_singleton<GuiStateComponent>();

    scene.get_ecs()
        .system( "GuiPlugin_Init" )
        .with<GuiStateComponent>()
        .in( EcsSystemPhase::INIT )
        .order( 10 )
        .run( []( EcsIterState& it ) {
            auto io    = it.world().get_singleton<IOServices>();
            auto vid   = it.world().get_singleton<VideoServices>();
            auto state = it.get_component<GuiStateComponent>();

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

            StyleColorsEditor();
            StyleSizesEditor();

            ImGuiStyle& style = ImGui::GetStyle();
            // https://github.com/ocornut/imgui/issues/8271#issuecomment-2564954070
            // Go through every colour and convert it to linear
            // This is because ImGui uses linear colours but we are using sRGB
            // This is a simple approximation of the conversion
            for (int i = 0; i < ImGuiCol_COUNT; i++) {
                /*float linear = (srgb <= 0.04045f) ? srgb / 12.92f : pow((srgb + 0.055f)
                 * / 1.055f, 2.4f);*/

                ImVec4& col = style.Colors[i];
                col.x       = col.x <= 0.04045f ? col.x / 12.92f : pow( ( col.x + 0.055f ) / 1.055f, 2.4f );
                col.y       = col.y <= 0.04045f ? col.y / 12.92f : pow( ( col.y + 0.055f ) / 1.055f, 2.4f );
                col.z       = col.z <= 0.04045f ? col.z / 12.92f : pow( ( col.z + 0.055f ) / 1.055f, 2.4f );
            }

            auto tmpl_rid = io->Resources->load( "materials/canvas.material" );
            auto tmpl     = io->Resources->get<MaterialTemplate>( tmpl_rid );
            NC_VERIFY( tmpl );
            auto mat = vid->Renderer->material_create( *tmpl );

            state->Material = mat; // TODO: why are we even storing the mat in the global state when
                                   // we're creating a dedicated entity material down below?

            // ctx.world()
            //     .entity( "ImGui_Material" )
            //     .with<MaterialComponent>( { tmpl_rid, mat, {} } )
            //     .child_of( state_id )
            //     .build();
        } );

    scene.get_ecs()
        .observer( "GuiPlugin_Cleanup" )
        .on<GuiStateComponent>( EcsCoreEvent::OnRemove )
        .run( []( EcsIterState& it ) {
            auto state = it.get_component<GuiStateComponent>();
            auto vid   = it.world().get_singleton<VideoServices>();

            // The editor plugin may have already nulled the current context
            // during its own cleanup, so make the GUI context current again
            // before touching any ImGui global state.
            if (!state->ImGuiCtx)
                return;

            ImGui::SetCurrentContext( state->ImGuiCtx );

            ImGuiPlatformIO& io = ImGui::GetPlatformIO();
            for (ImTextureData* tex : io.Textures) {
                if (tex->BackendUserData) {
                    RID rid( static_cast<uint64_t>( reinterpret_cast<uintptr_t>( tex->BackendUserData ) ) );
                    vid->Renderer->destroy_rid( rid );
                }
                tex->BackendUserData = nullptr;
                tex->SetTexID( ImTextureID_Invalid );
                tex->SetStatus( ImTextureStatus_Destroyed );
            }

            auto fonts = ImGui::GetIO().Fonts;
            NC_LOG_DEBUG_C( log::GUI, "Destroying font atlas TexID={}", fonts->TexID.GetTexID() );
            fonts->Clear();

            NC_LOG_DEBUG_C( log::GUI, "Destroying ImGui context" );
            ImGui::DestroyContext();
        } );

    scene.get_ecs()
        .system( "GuiPlugin_ProcessEvents" )
        .in( EcsSystemPhase::PRE_FRAME )
        .with<GuiStateComponent>()
        .run( []( EcsIterState& it ) {
            auto io  = it.world().get_singleton<IOServices>();
            auto vid = it.world().get_singleton<VideoServices>();

            ImGuiIO& gui_io  = ImGui::GetIO();
            gui_io.DeltaTime = static_cast<float>( it.delta_time() );

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

            for (const auto& ev : vid->Window->window_events()) {
                if (auto focus = std::get_if<WindowFocusEvent>( &ev )) {
                    gui_io.AddFocusEvent( focus->focused );
                }
            }

            // FIXME: improve this. shouldn't access SDL directly, delegate to InputService/EcsInputFeature
            // SDL_Window* sdl_window =
            //    SDL_GetWindowFromID( static_cast<SDL_WindowID>( vid->Window->get_main_window_id() ) );
            // if (sdl_window) {
            //    if (gui_io.WantTextInput) {
            //        SDL_StartTextInput( sdl_window );
            //    } else {
            //        SDL_StopTextInput( sdl_window );
            //    }
            //}
        } );

    scene.get_ecs()
        .system( "GuiPlugin_PrepareFrame" )
        .with<MainWindowTag, WindowComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( EcsIterState& it ) {
            auto win = it.get_component<WindowComponent>();
            auto vid = it.world().get_singleton<VideoServices>();

            Vec2i size = vid->Renderer->swapchain_get_size( win->Swapchain );

            ImGuiIO& io      = ImGui::GetIO();
            io.DisplaySize.x = size.x;
            io.DisplaySize.y = size.y;

            ImGui::NewFrame();
        } );

    scene.get_ecs()
        .system( "GuiPlugin_EndFrame" )
        .with<MainWindowTag, WindowComponent>()
        .in( EcsSystemPhase::POST_UPDATE )
        .each( []( EcsIterState& it ) {
            auto vid   = it.world().get_singleton<VideoServices>();
            auto state = it.world().get_singleton<GuiStateComponent>();

            ImGui::Render();

            auto wanted_cursor = state->CursorMap.at( ImGui::GetMouseCursor() );
            vid->Window->set_cursor_type( wanted_cursor );

            ImDrawData* dd = ImGui::GetDrawData();
            if (!dd || dd->DisplaySize.x <= 0 || dd->DisplaySize.y <= 0 || !state->Material) {
                NC_LOG_TRACE_C(
                    log::GRAPHICS, "GuiPlugin_EndFrame: skip (dd={} disp={:.0f}x{:.0f} mat_valid={})",
                    static_cast<void*>( dd ), dd ? dd->DisplaySize.x : 0, dd ? dd->DisplaySize.y : 0,
                    state->Material.is_valid()
                );
                return;
            }
            NC_LOG_TRACE_C(
                log::GRAPHICS, "GuiPlugin_EndFrame: {} cmd lists, {} total vertices", dd->CmdListsCount,
                dd->TotalVtxCount
            );

            auto handle_tex = [vid]( ImTextureData* tex ) {
                switch (tex->Status) {
                    case ImTextureStatus_WantCreate: {
                        Image image( tex->Width, tex->Height, tex->GetPixels() );
                        RID rid = vid->Renderer->texture_2d_create( image );
                        tex->SetTexID( reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( rid.value ) ) );
                        tex->BackendUserData = reinterpret_cast<void*>( static_cast<uintptr_t>( rid.value ) );
                        tex->SetStatus( ImTextureStatus_OK );
                        break;
                    }
                    case ImTextureStatus_WantDestroy: {
                        RID rid( reinterpret_cast<uintptr_t>( tex->BackendUserData ) );
                        vid->Renderer->destroy_rid( rid );
                        tex->BackendUserData = nullptr;
                        tex->SetTexID( ImTextureID_Invalid );
                        tex->SetStatus( ImTextureStatus_Destroyed );
                        break;
                    }
                    case ImTextureStatus_WantUpdates: {
                        RID old_rid( reinterpret_cast<uintptr_t>( tex->BackendUserData ) );
                        vid->Renderer->destroy_rid( old_rid );
                        Image image( tex->Width, tex->Height, tex->GetPixels() );
                        RID new_rid = vid->Renderer->texture_2d_create( image );
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
                for (int i = 0; i < cmd_list->CmdBuffer.Size; i++) {
                    auto& cmd = cmd_list->CmdBuffer[i];
                    if (cmd.UserCallback) {
                        NC_LOG_TRACE_C( log::GRAPHICS, "    UserCallback cmd" );
                        cmd.UserCallback( cmd_list, &cmd );
                        continue;
                    }
                    if (cmd.ElemCount == 0)
                        continue;

                    Rect2i clip_rect(
                        static_cast<int>( ( cmd.ClipRect.x - dd->DisplayPos.x ) * dd->FramebufferScale.x ),
                        static_cast<int>( ( cmd.ClipRect.y - dd->DisplayPos.y ) * dd->FramebufferScale.y ),
                        static_cast<int>( ( cmd.ClipRect.z - cmd.ClipRect.x ) * dd->FramebufferScale.x ),
                        static_cast<int>( ( cmd.ClipRect.w - cmd.ClipRect.y ) * dd->FramebufferScale.y )
                    );
                    clip_rect.x = std::max( clip_rect.x, 0 );
                    clip_rect.y = std::max( clip_rect.y, 0 );
                    clip_rect.w = std::max( clip_rect.w, 0 );
                    clip_rect.h = std::max( clip_rect.h, 0 );

                    auto vtx        = reinterpret_cast<Vertex2D*>( cmd_list->VtxBuffer.Data + cmd.VtxOffset );
                    auto idx        = reinterpret_cast<const ImDrawIdx*>( cmd_list->IdxBuffer.Data + cmd.IdxOffset );
                    auto vert_count = static_cast<size_t>( cmd_list->VtxBuffer.Size - cmd.VtxOffset );
                    auto idx_count  = static_cast<size_t>( cmd.ElemCount );

                    NC_LOG_TRACE_C(
                        log::GRAPHICS, "  canvas_draw_triangles: {} verts, {} idx, clip={},{} {}x{}", vert_count,
                        idx_count, clip_rect.x, clip_rect.y, clip_rect.w, clip_rect.h
                    );

                    // NOTE: we don't directly set the material texture here, because we only have one
                    // shared material for all canvas draw, and doing that will just replace the tex with
                    // whatever is last set. (mistake learned)

                    RID tex_id = reinterpret_cast<uintptr_t>( cmd.GetTexID() );
                    // Push a new draw of canvas primitive with its own texture override
                    vid->Renderer->canvas_draw_triangles(
                        { vtx, vert_count }, { idx, idx_count }, state->Material, tex_id, clip_rect
                    );
                }
            }
        } );
}

//------------------------------------------------------------------------------

void unregister_gui_plugin( Scene& scene )
{
    scene.get_ecs().remove_singleton<GuiStateComponent>();
}

} // namespace nc::editor
