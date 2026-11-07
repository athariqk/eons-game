#include "gui_plugin.h"

#include <cstring>

#include <ncore/runtime/components.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/io/resource_loader.h>
#include <ncore/runtime/resources/font.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/video/render_service.h>
#include <ncore/services/video/window_service.h>

#include <editor/backends/imgui/imgui_style.h>
#include <editor/backends/imgui/imgui_utils.h>

namespace sea::editor {

void register_gui_plugin( nc::Scene& scene )
{
    scene.get_ecs().add_singleton<EditorGuiComponent>();

    scene.get_ecs()
        .system( "GuiPlugin_Init" )
        .with<EditorGuiComponent>()
        .in( nc::EcsSystemPhase::INIT )
        .order( 10 )
        .run( [&scene]( nc::EcsIterState& it ) {
            auto io    = it.world().get_singleton<nc::IoServices>();
            auto vid   = it.world().get_singleton<nc::VideoServices>();
            auto state = it.get_component<EditorGuiComponent>();

            IMGUI_CHECKVERSION();
            state->imgui_ctx  = ImGui::CreateContext();
            ImGuiIO& imgui_io = ImGui::GetIO();
            imgui_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            imgui_io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_RendererHasVtxOffset |
                                     ImGuiBackendFlags_RendererHasTextures;
            // Fonts live in assets/fonts/*.ttf.bin, so they can only be handed to
            // ImGui from memory — the raw TTF no longer ships in the runtime tree.
            // The buffer must come from IM_ALLOC: ImGui frees it with IM_FREE (free),
            // while BytesBuffer allocates through NcAllocator/mimalloc outside ASAN.
            auto load_font = [&]( const char* p_path ) -> ImFont* {
                auto font = io->resources->load<nc::Font>( p_path );
                if (!font)
                    return nullptr;
                auto bytes = font->get_data();
                if (bytes.size() == 0)
                    return nullptr;
                void* buf = IM_ALLOC( bytes.size() );
                if (!buf)
                    return nullptr;
                std::memcpy( buf, bytes.data(), bytes.size() );
                return imgui_io.Fonts->AddFontFromMemoryTTF( buf, static_cast<int>( bytes.size() ) );
            };

            load_font( "fonts/SpaceGrotesk-SemiBold.ttf" );
            load_font( "fonts/SpaceGrotesk-Regular.ttf" );
            imgui_io.FontDefault = load_font( "fonts/SpaceGrotesk-Medium.ttf" );
            if (imgui_io.Fonts->Fonts.empty())
                imgui_io.FontDefault = imgui_io.Fonts->AddFontDefault();

            for (int i = 0; i < ImGuiMouseCursor_COUNT; i++) {
                auto imgui_cursor               = static_cast<ImGuiMouseCursor>( i );
                auto cursor_type                = cursor_type_to_imgui_cursor( imgui_cursor );
                state->cursor_map[imgui_cursor] = cursor_type;
            }

            ImGui::SetCurrentContext( state->imgui_ctx );

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

            auto shader        = io->resources->load<nc::MaterialShader>( "shaders/materials/standard_canvas.slang" );
            state->canvas_item = vid->renderer->canvas_item_create();
            if (shader) {
                // The material owns its GPU instance; the component keeps it alive.
                state->canvas_material = nc::Ref<nc::Material>::create();
                state->canvas_material->set_shader( shader );
                vid->renderer->canvas_item_set_material( state->canvas_item, state->canvas_material->prepare( scene ) );
            }
        } );

    scene.get_ecs()
        .observer( "GuiPlugin_Cleanup" )
        .on<EditorGuiComponent>( nc::ecs::ON_REMOVE )
        .run( []( nc::EcsIterState& it ) {
            auto state = it.get_component<EditorGuiComponent>();
            auto vid   = it.world().get_singleton<nc::VideoServices>();

            // The editor plugin may have already nulled the current context
            // during its own cleanup, so make the GUI context current again
            // before touching any ImGui global state.
            if (!state->imgui_ctx)
                return;

            ImGui::SetCurrentContext( state->imgui_ctx );

            ImGuiPlatformIO& io = ImGui::GetPlatformIO();
            for (ImTextureData* tex : io.Textures) {
                if (tex->BackendUserData) {
                    nc::RID rid( static_cast<uint64_t>( reinterpret_cast<uintptr_t>( tex->BackendUserData ) ) );
                    vid->renderer->destroy_rid( rid );
                }
                tex->BackendUserData = nullptr;
                tex->SetTexID( ImTextureID_Invalid );
                tex->SetStatus( ImTextureStatus_Destroyed );
            }

            if (state->canvas_item)
                vid->renderer->destroy_rid( state->canvas_item );

            state->canvas_material = nullptr;

            auto fonts = ImGui::GetIO().Fonts;
            NC_LOG_DEBUG_C( nc::log::GUI, "Destroying font atlas TexID={}", fonts->TexID.GetTexID() );
            fonts->Clear();

            NC_LOG_DEBUG_C( nc::log::GUI, "Destroying ImGui context" );
            ImGui::DestroyContext();
        } );

    scene.get_ecs()
        .system( "GuiPlugin_ProcessEvents" )
        .in( nc::EcsSystemPhase::PRE_FRAME )
        .with<EditorGuiComponent>()
        .run( []( nc::EcsIterState& it ) {
            auto io  = it.world().get_singleton<nc::IoServices>();
            auto vid = it.world().get_singleton<nc::VideoServices>();

            ImGuiIO& gui_io  = ImGui::GetIO();
            gui_io.DeltaTime = static_cast<float>( it.delta_time() );

            for (const auto& ev : io->inputs->get_events()) {
                std::visit(
                    [&]( auto&& e ) {
                        using T = std::decay_t<decltype( e )>;
                        if constexpr (std::is_same_v<T, nc::MouseMotionEvent>) {
                            gui_io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
                            gui_io.AddMousePosEvent( e.position.x, e.position.y );
                        } else if constexpr (std::is_same_v<T, nc::MouseWheelEvent>) {
                            gui_io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
                            gui_io.AddMouseWheelEvent( e.scroll_x, e.scroll_y );
                        } else if constexpr (std::is_same_v<T, nc::MouseButtonEvent>) {
                            int button = -1;
                            if (e.button == nc::ButtonIndex::LEFT)
                                button = 0;
                            else if (e.button == nc::ButtonIndex::RIGHT)
                                button = 1;
                            else if (e.button == nc::ButtonIndex::MIDDLE)
                                button = 2;
                            if (button != -1) {
                                gui_io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
                                gui_io.AddMouseButtonEvent( button, e.action == nc::ButtonAction::PRESS );
                            }
                        } else if constexpr (std::is_same_v<T, nc::TextInputEvent>) {
                            gui_io.AddInputCharactersUTF8( e.text );
                        } else if constexpr (std::is_same_v<T, nc::KeyEvent>) {
                            ImGuiKey key = key_to_imgui_key( e.key );
                            if (key != ImGuiKey_None) {
                                bool down = e.action == nc::ButtonAction::PRESS;
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

            for (const auto& ev : vid->window->window_events()) {
                if (auto focus = std::get_if<nc::WindowFocusEvent>( &ev )) {
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
            // !!!! HACKKKKK
            if (gui_io.WantTextInput) {
                vid->window->window_text_input_start( vid->window->get_main_window_id() );
            } else {
                vid->window->window_text_input_stop( vid->window->get_main_window_id() );
            }
        } );

    scene.get_ecs()
        .system( "GuiPlugin_PrepareFrame" )
        .with<nc::MainWindowTag, nc::WindowComponent>()
        .in( nc::EcsSystemPhase::PRE_UPDATE )
        // Must run before every other PRE_UPDATE system that touches ImGui: it is
        // what publishes DisplaySize and starts the frame. Without this the
        // viewport still reports WorkSize=1x1 and docking layouts bake 1px nodes.
        .order( -1000 )
        .each( []( nc::EcsIterState& it ) {
            auto win = it.get_component<nc::WindowComponent>();
            auto vid = it.world().get_singleton<nc::VideoServices>();

            nc::Vec2i size = vid->renderer->swapchain_get_size( win->swapchain );

            ImGuiIO& io      = ImGui::GetIO();
            io.DisplaySize.x = static_cast<float>( size.x );
            io.DisplaySize.y = static_cast<float>( size.y );

            ImGui::NewFrame();
        } );

    scene.get_ecs()
        .system( "GuiPlugin_EndFrame" )
        .with<nc::MainWindowTag, nc::WindowComponent>()
        .in( nc::EcsSystemPhase::POST_UPDATE )
        .each( []( nc::EcsIterState& it ) {
            auto vid   = it.world().get_singleton<nc::VideoServices>();
            auto state = it.world().get_singleton<EditorGuiComponent>();

            ImGui::Render();

            auto wanted_cursor = state->cursor_map.at( ImGui::GetMouseCursor() );
            vid->window->set_cursor_type( wanted_cursor );

            ImDrawData* dd = ImGui::GetDrawData();
            if (!dd || dd->DisplaySize.x <= 0 || dd->DisplaySize.y <= 0 || !state->canvas_item) {
                NC_LOG_TRACE_C(
                    nc::log::GRAPHICS, "GuiPlugin_EndFrame: skip (dd={} disp={:.0f}x{:.0f}", static_cast<void*>( dd ),
                    dd ? dd->DisplaySize.x : 0, dd ? dd->DisplaySize.y : 0
                );
                return;
            }
            NC_LOG_TRACE_C(
                nc::log::GRAPHICS, "GuiPlugin_EndFrame: {} cmd lists, {} total vertices", dd->CmdListsCount,
                dd->TotalVtxCount
            );

            auto handle_tex = [vid]( ImTextureData* tex ) {
                switch (tex->Status) {
                    case ImTextureStatus_WantCreate: {
                        NC_ASSERT_MSG( tex->Format == ImTextureFormat_RGBA32, "ImGui Alpha8 textures are unsupported" );

                        const auto width  = static_cast<uint32_t>( tex->Width );
                        const auto height = static_cast<uint32_t>( tex->Height );
                        nc::Texture2DDesc desc{};
                        desc.width  = width;
                        desc.height = height;
                        desc.format = nc::gfx::TextureFormat::RGBA8_UNORM_SRGB;
                        desc.data   = nc::Span<const std::byte>(
                            reinterpret_cast<const std::byte*>( tex->GetPixels() ),
                            static_cast<size_t>( width ) * height * 4
                        );
                        desc.updatable  = true;
                        desc.debug_name = "imgui_texture";

                        nc::RID rid = vid->renderer->texture_2d_create( desc );
                        tex->SetTexID( reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( rid.value ) ) );
                        tex->BackendUserData = reinterpret_cast<void*>( static_cast<uintptr_t>( rid.value ) );
                        tex->SetStatus( ImTextureStatus_OK );
                        break;
                    }
                    case ImTextureStatus_WantDestroy: {
                        nc::RID rid( reinterpret_cast<uintptr_t>( tex->BackendUserData ) );
                        vid->renderer->destroy_rid( rid );
                        tex->BackendUserData = nullptr;
                        tex->SetTexID( ImTextureID_Invalid );
                        tex->SetStatus( ImTextureStatus_Destroyed );
                        break;
                    }
                    case ImTextureStatus_WantUpdates: {
                        nc::RID rid( reinterpret_cast<uintptr_t>( tex->BackendUserData ) );
                        const auto pitch = static_cast<uint32_t>( tex->GetPitch() );
                        if (tex->Updates.Size > 0) {
                            for (int r = 0; r < tex->Updates.Size; ++r) {
                                const ImTextureRect& rect = tex->Updates[r];
                                vid->renderer->texture_data_update(
                                    rid, rect.x, rect.y, rect.w, rect.h, pitch, tex->GetPixelsAt( rect.x, rect.y )
                                );
                            }
                        } else if (tex->UpdateRect.w > 0 && tex->UpdateRect.h > 0) {
                            const ImTextureRect& rect = tex->UpdateRect;
                            vid->renderer->texture_data_update(
                                rid, rect.x, rect.y, rect.w, rect.h, pitch, tex->GetPixelsAt( rect.x, rect.y )
                            );
                        }
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

            uint32_t z_order = 0;
            for (auto cmd_list : dd->CmdLists) {
                NC_LOG_TRACE_C(
                    nc::log::GRAPHICS, "  CmdList: {} cmds, {} vtx, {} idx", cmd_list->CmdBuffer.Size,
                    cmd_list->VtxBuffer.Size, cmd_list->IdxBuffer.Size
                );

                for (int i = 0; i < cmd_list->CmdBuffer.Size; i++) {
                    auto& cmd = cmd_list->CmdBuffer[i];
                    if (cmd.UserCallback) {
                        NC_LOG_TRACE_C( nc::log::GRAPHICS, "    UserCallback cmd" );
                        cmd.UserCallback( cmd_list, &cmd );
                        continue;
                    }
                    if (cmd.ElemCount == 0)
                        continue;

                    nc::Rect2i clip_rect(
                        static_cast<int>( ( cmd.ClipRect.x - dd->DisplayPos.x ) * dd->FramebufferScale.x ),
                        static_cast<int>( ( cmd.ClipRect.y - dd->DisplayPos.y ) * dd->FramebufferScale.y ),
                        static_cast<int>( ( cmd.ClipRect.z - cmd.ClipRect.x ) * dd->FramebufferScale.x ),
                        static_cast<int>( ( cmd.ClipRect.w - cmd.ClipRect.y ) * dd->FramebufferScale.y )
                    );
                    clip_rect.x = std::max( clip_rect.x, 0 );
                    clip_rect.y = std::max( clip_rect.y, 0 );
                    clip_rect.w = std::max( clip_rect.w, 0 );
                    clip_rect.h = std::max( clip_rect.h, 0 );

                    auto vtx        = reinterpret_cast<nc::Vertex2D*>( cmd_list->VtxBuffer.Data + cmd.VtxOffset );
                    auto idx        = reinterpret_cast<const ImDrawIdx*>( cmd_list->IdxBuffer.Data + cmd.IdxOffset );
                    auto vert_count = static_cast<size_t>( cmd_list->VtxBuffer.Size - cmd.VtxOffset );
                    auto idx_count  = static_cast<size_t>( cmd.ElemCount );

                    NC_LOG_TRACE_C(
                        nc::log::GRAPHICS, "  ImGui draw cmd: {} verts, {} idx, clip={},{} {}x{}", vert_count,
                        idx_count, clip_rect.x, clip_rect.y, clip_rect.w, clip_rect.h
                    );

                    // NOTE: we don't directly set the material texture here, because we only have one
                    // shared material for all canvas draw, and doing that will just replace the tex with
                    // whatever is last set. (mistake learned)

                    nc::RID tex_id = reinterpret_cast<uintptr_t>( cmd.GetTexID() );
                    // Push a new draw of canvas primitive with its own texture override. The
                    // incrementing z-order preserves ImGui's back-to-front draw order.
                    vid->renderer->canvas_item_draw(
                        state->canvas_item, { vtx, vert_count },
                        { reinterpret_cast<const uint32_t*>( idx ), idx_count }, tex_id, clip_rect, z_order++
                    );
                }
            }
        } );
}

//------------------------------------------------------------------------------

void unregister_gui_plugin( nc::Scene& scene )
{
    scene.get_ecs().remove_singleton<EditorGuiComponent>();
}

} // namespace sea::editor
