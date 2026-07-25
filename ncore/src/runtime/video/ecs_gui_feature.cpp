#include "ecs_gui_feature.h"

#include <imgui.h>

#include <ncore/modules/input/input_event.h>
#include <ncore/modules/input/input_module.h>
#include <ncore/modules/io/resource_manager.h>
#include <ncore/modules/module_registry.h>
#include <ncore/modules/video/render_module.h>
#include <ncore/modules/video/window/window_types.h>
#include <ncore/modules/video/window_module.h>
#include <ncore/resources/image.h>
#include <ncore/resources/material_template.h>
#include <ncore/runtime/components/ecs_material.h>
#include <ncore/runtime/components/ecs_window.h>
#include <ncore/runtime/ecs_base_features.h>
#include <ncore/runtime/ecs_system.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

inline static CursorType map_cursor_type( ImGuiMouseCursor cursor )
{
    switch (cursor) {
        case ImGuiMouseCursor_Arrow:
            return CursorType::DEFAULT;
        case ImGuiMouseCursor_TextInput:
            return CursorType::TEXT;
        case ImGuiMouseCursor_ResizeNS:
            return CursorType::RESIZE_NS;
        case ImGuiMouseCursor_ResizeEW:
            return CursorType::RESIZE_EW;
        case ImGuiMouseCursor_ResizeNESW:
            return CursorType::RESIZE_NESW;
        case ImGuiMouseCursor_ResizeNWSE:
            return CursorType::RESIZE_NWSE;
        case ImGuiMouseCursor_Hand:
            return CursorType::POINTER;
        case ImGuiMouseCursor_Wait:
            return CursorType::WAIT;
        default:
            return CursorType::DEFAULT;
    }
}

inline static ImGuiKey KeyToImGuiKey( Key key )
{
    switch (key) {
        case Key::W:
            return ImGuiKey_W;
        case Key::A:
            return ImGuiKey_A;
        case Key::S:
            return ImGuiKey_S;
        case Key::D:
            return ImGuiKey_D;
        case Key::UP:
            return ImGuiKey_UpArrow;
        case Key::DOWN:
            return ImGuiKey_DownArrow;
        case Key::LEFT:
            return ImGuiKey_LeftArrow;
        case Key::RIGHT:
            return ImGuiKey_RightArrow;
        case Key::SPACE:
            return ImGuiKey_Space;
        case Key::ENTER:
            return ImGuiKey_Enter;
        case Key::ESC:
            return ImGuiKey_Escape;
        case Key::SHIFT:
            return ImGuiKey_LeftShift;
        case Key::CTRL:
            return ImGuiKey_LeftCtrl;
        case Key::ALT:
            return ImGuiKey_LeftAlt;
        case Key::TAB:
            return ImGuiKey_Tab;
        case Key::BKSP:
            return ImGuiKey_Backspace;
        case Key::F5:
            return ImGuiKey_F5;
        case Key::UNKNOWN:
            return ImGuiKey_None;
    }
    return ImGuiKey_None;
}

struct ImGuiState {
    InputModule* input      = nullptr;
    ImGuiContext* imgui_ctx = nullptr;
    HashMap<ImGuiMouseCursor, CursorType> cursor_map;
    RID material;
    RID last_tex_id;

    NSTRUCT(
        ImGuiState, NC_F( ImGuiState, input ) NC_F( ImGuiState, imgui_ctx ) NC_F( ImGuiState, cursor_map )
                        NC_F( ImGuiState, material )
    )
};

void EcsGuiFeature::build( EcsWorld& world )
{
    world.emplace_singleton<ImGuiState>();

    world.create_system( "EcsGuiFeature::Init" ).in( EcsSystemPhase::INIT ).order( 10 ).run( []( QueryContext& ctx ) {
        auto io  = ctx.world().get_singleton<IoModules>();
        auto gfx = ctx.world().get_singleton<GraphicsModules>();

        ImGuiState state{};
        state.input = ctx.modules().resolve<InputModule>();

        IMGUI_CHECKVERSION();
        state.imgui_ctx = ImGui::CreateContext();
        ImGui::StyleColorsDark();
        auto& Colors = ImGui::GetStyle().Colors;
        for (int i = 0; i < ImGuiCol_COUNT; ++i) {
            ImVec4& Col{ Colors[i] };
            Col.x = Col.x > 0 ? std::pow( Col.x, 0.5f ) : 0;
            Col.y = Col.y > 0 ? std::pow( Col.y, 0.5f ) : 0;
            Col.z = Col.z > 0 ? std::pow( Col.z, 0.5f ) : 0;
            Col.w = Col.w > 0 ? std::pow( Col.w, 0.5f ) : 0;
        }
        Colors[ImGuiCol_WindowBg].w = 0.75f;
        Colors[ImGuiCol_PlotLines]  = { 1.00f, 1.00f, 1.00f, 1.00f };

        ImGuiIO& imgui_io = ImGui::GetIO();
        imgui_io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_RendererHasVtxOffset |
                                 ImGuiBackendFlags_RendererHasTextures;

        for (int i = 0; i < ImGuiMouseCursor_COUNT; i++) {
            auto imgui_cursor              = static_cast<ImGuiMouseCursor>( i );
            auto cursor_type               = map_cursor_type( imgui_cursor );
            state.cursor_map[imgui_cursor] = cursor_type;
        }

        ImGui::SetCurrentContext( state.imgui_ctx );

        auto tmpl = io->resources->load<MaterialTemplate>( "materials/canvas.material" );
        NC_ASSERT_NULL( tmpl );
        auto mat = gfx->renderer->material_create( *tmpl );

        state.material = mat;

        auto state_id = ctx.world().set_singleton<ImGuiState>( state );

        ctx.world()
            .create_entity( "ImGui_Material" )
            .with<EcsMaterialInstance>( EcsMaterialInstance{ .template_ref = tmpl, .material = mat, .textures = {} } )
            .child_of( state_id )
            .build();
    } );

    world.create_observer( "EcsGuiFeature::Cleanup" )
        .on<ImGuiState>( EcsCoreEvent::OnRemove )
        .run( []( QueryContext& ctx ) {
            auto state          = ctx.get_component<ImGuiState>();
            auto gfx            = ctx.world().get_singleton<GraphicsModules>();
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

            if (state->imgui_ctx)
                ImGui::DestroyContext( state->imgui_ctx );
        } );

    world.create_system( "EcsGuiFeature::ProcessEvents" )
        .in( EcsSystemPhase::PRE_FRAME )
        .with<ImGuiState>()
        .run( []( QueryContext& ctx ) {
            auto gfx   = ctx.world().get_singleton<GraphicsModules>();
            auto state = ctx.world().get_singleton<ImGuiState>();

            ImGuiIO& io  = ImGui::GetIO();
            io.DeltaTime = static_cast<float>( ctx.delta_time() );

            for (const auto& ev : state->input->input_events()) {
                std::visit(
                    [&]( auto&& e ) {
                        using T = std::decay_t<decltype( e )>;
                        if constexpr (std::is_same_v<T, MouseMotionEvent>) {
                            io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
                            io.AddMousePosEvent( e.position.X, e.position.Y );
                        } else if constexpr (std::is_same_v<T, MouseWheelEvent>) {
                            io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
                            io.AddMouseWheelEvent( e.scroll_x, e.scroll_y );
                        } else if constexpr (std::is_same_v<T, MouseButtonEvent>) {
                            int button = -1;
                            if (e.button == ButtonIndex::LEFT)
                                button = 0;
                            else if (e.button == ButtonIndex::RIGHT)
                                button = 1;
                            else if (e.button == ButtonIndex::MIDDLE)
                                button = 2;
                            if (button != -1) {
                                io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
                                io.AddMouseButtonEvent( button, e.action == ButtonAction::PRESS );
                            }
                        } else if constexpr (std::is_same_v<T, TextInputEvent>) {
                            io.AddInputCharactersUTF8( e.text );
                        } else if constexpr (std::is_same_v<T, KeyEvent>) {
                            ImGuiKey key = KeyToImGuiKey( e.key );
                            if (key != ImGuiKey_None) {
                                bool down = e.action == ButtonAction::PRESS;
                                io.AddKeyEvent( key, down );
                                io.SetKeyEventNativeData(
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
                    io.AddFocusEvent( focus->focused );
                }
            }
        } );

    world.create_system( "EcsGuiFeature::PrepareFrame" )
        .in( EcsSystemPhase::PRE_UPDATE )
        .with<EcsSwapChainRef>()
        .run( []( QueryContext& ctx ) {
            auto rd = ctx.get_component<EcsSwapChainRef>();

            Vec2 size = rd->size;
            if (size.is_zero())
                return;

            ImGuiIO& io      = ImGui::GetIO();
            io.DisplaySize.x = size.X;
            io.DisplaySize.y = size.Y;

            ImGui::NewFrame();
        } );

    world.create_system( "EcsGuiFeature::EndFrame" )
        .in( EcsSystemPhase::POST_UPDATE )
        .with<EcsSwapChainRef>()
        .run( []( QueryContext& ctx ) {
            auto gfx   = ctx.world().get_singleton<GraphicsModules>();
            auto state = ctx.world().get_singleton<ImGuiState>();

            ImGui::Render();

            auto wanted_cursor = state->cursor_map.at( ImGui::GetMouseCursor() );
            gfx->window->set_cursor_type( wanted_cursor );

            ImDrawData* dd = ImGui::GetDrawData();
            if (!dd || dd->DisplaySize.x <= 0 || dd->DisplaySize.y <= 0 || !state->material.is_valid()) {
                NC_LOG_TRACE_C(
                    log::GRAPHICS, "EcsGuiFeature::EndFrame: skip (dd={} disp={:.0f}x{:.0f} mat_valid={})",
                    static_cast<void*>( dd ), dd ? dd->DisplaySize.x : 0, dd ? dd->DisplaySize.y : 0,
                    state->material.is_valid()
                );
                return;
            }
            NC_LOG_TRACE_C(
                log::GRAPHICS, "EcsGuiFeature::EndFrame: {} cmd lists, {} total vertices", dd->CmdListsCount,
                dd->TotalVtxCount
            );

            auto handle_tex = [&gfx]( ImTextureData* tex ) {
                switch (tex->Status) {
                    case ImTextureStatus_WantCreate: {
                        Image image( tex->Width, tex->Height, tex->GetPixels() );
                        RID rid = gfx->renderer->create_texture_2d( image );
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
                        RID new_rid = gfx->renderer->create_texture_2d( image );
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

                    Vec4 clip_rect = Vec4(
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
                        vert_count, idx_count, clip_rect.X, clip_rect.Y, clip_rect.W, clip_rect.H
                    );
                    gfx->renderer->canvas_draw_triangles(
                        { vtx, vert_count }, { idx, idx_count }, state->material, clip_rect
                    );
                }
            }
        } );
}

} // namespace nc
