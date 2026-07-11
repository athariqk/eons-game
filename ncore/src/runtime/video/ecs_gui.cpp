#include "ecs_gui.h"

#include <imgui.h>

#include <ncore/game_world.h>
#include <ncore/modules/input/input_event.h>
#include <ncore/modules/input/input_module.h>
#include <ncore/modules/module_registry.h>
#include <ncore/modules/video/graphics_module.h>
#include <ncore/modules/video/resources/image.h>
#include <ncore/modules/video/video_module.h>
#include <ncore/modules/video/window_types.h>
#include <ncore/runtime/components/ecs_window.h>
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
        case Key::UNKNOWN:
            return ImGuiKey_None;
    }
    return ImGuiKey_None;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
struct ImGuiState {
    VideoModule* video      = nullptr;
    GraphicsModule* gfx     = nullptr;
    InputModule* input      = nullptr;
    ImGuiContext* imgui_ctx = nullptr;
    UnorderedMap<ImGuiMouseCursor, CursorType> cursor_map;

    ImGuiState();
    ~ImGuiState();
    ImGuiState& operator=( const ImGuiState& state );

    NSTRUCT(
        ImGuiState, NC_F( ImGuiState, video ) NC_F( ImGuiState, gfx ) NC_F( ImGuiState, input )
                        NC_F( ImGuiState, imgui_ctx ) NC_F( ImGuiState, cursor_map )
    )
};
#pragma GCC diagnostic pop

ImGuiState::ImGuiState()
{
    IMGUI_CHECKVERSION();
    imgui_ctx = ImGui::CreateContext();
    ImGui::StyleColorsClassic();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;

    for (int i = 0; i < ImGuiMouseCursor_COUNT; i++) {
        auto imgui_cursor        = static_cast<ImGuiMouseCursor>( i );
        auto cursor_type         = map_cursor_type( imgui_cursor );
        cursor_map[imgui_cursor] = cursor_type;
    }
}

ImGuiState::~ImGuiState()
{
    ImGuiPlatformIO& io = ImGui::GetPlatformIO();
    for (ImTextureData* tex : io.Textures) {
        if (tex->BackendUserData) {
            RID rid( static_cast<uint64_t>( reinterpret_cast<uintptr_t>( tex->BackendUserData ) ) );
            gfx->destroy_resource( rid );
        }
        tex->BackendUserData = nullptr;
        tex->SetTexID( ImTextureID_Invalid );
        tex->SetStatus( ImTextureStatus_Destroyed );
    }

    if (imgui_ctx)
        ImGui::DestroyContext( imgui_ctx );
}

ImGuiState& ImGuiState::operator=( const ImGuiState& state )
{
    video      = state.video;
    gfx        = state.gfx;
    input      = state.input;
    imgui_ctx  = state.imgui_ctx;
    cursor_map = state.cursor_map;
    return *this;
}

void EcsGuiFeature::build( EcsWorld& world )
{
    world.emplace_singleton<ImGuiState>();

    world.create_system( "EcsGuiFeature::Init" )
        .in( EcsSystemPhase::Init )
        .with<ImGuiState>()
        .order( 10 )
        .run( []( QueryContext& ctx ) {
            auto state   = ctx.get_component<ImGuiState>();
            state->video = ctx.world().get_parent().get_modules().resolve<VideoModule>();
            state->gfx   = ctx.world().get_parent().get_modules().resolve<GraphicsModule>();
            state->input = ctx.world().get_parent().get_modules().resolve<InputModule>();
            ImGui::SetCurrentContext( state->imgui_ctx );
        } );

    world.create_system( "EcsGuiFeature::ProcessEvents" )
        .in( EcsSystemPhase::PreFrame )
        .with<ImGuiState>()
        .run( []( QueryContext& ctx ) {
            auto& state = ctx.world().get_singleton<ImGuiState>();

            ImGuiIO& io = ImGui::GetIO();

            // Poll input events and feed to ImGui IO
            for (const auto& ev : state.input->input_events()) {
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

            for (const auto& ev : state.video->window_events()) {
                if (auto focus = std::get_if<WindowFocusEvent>( &ev )) {
                    io.AddFocusEvent( focus->focused );
                }
            }
        } );

    world.create_system( "EcsGuiFeature::PrepareFrame" )
        .in( EcsSystemPhase::PreFrame )
        .with<EcsTargetSurface>()
        .run( []( QueryContext& ctx ) {
            auto rd         = ctx.get_component<EcsTargetSurface>();
            auto& state     = ctx.world().get_singleton<ImGuiState>();

            ImGuiIO& io      = ImGui::GetIO();
            auto size        = state.gfx->get_surface_size( rd->surface );
            io.DisplaySize.x = size.X;
            io.DisplaySize.y = size.Y;

            ImGui::NewFrame();
        } );

    world.create_system( "EcsGuiFeature::EndFrame" )
        .in( EcsSystemPhase::PostFrame )
        .with<EcsTargetSurface>()
        .run( []( QueryContext& ctx ) {
            auto& state = ctx.world().get_singleton<ImGuiState>();

            ImGui::Render();

            // sets the wanted cursor type, it should be safe to subscript directly
            // since we already done exhaustive mapping in init
            auto wanted_cursor = state.cursor_map.at( ImGui::GetMouseCursor() );
            state.video->set_cursor_type( wanted_cursor );

            ImDrawData* dd    = ImGui::GetDrawData();
            Vec2 display_size = Vec2( dd->DisplaySize.x, dd->DisplaySize.y );
            if (display_size.is_zero())
                return;

            auto handle_tex = [&state]( ImTextureData* tex ) {
                switch (tex->Status) {
                    case ImTextureStatus_WantCreate: {
                        Image image( tex->Width, tex->Height, tex->GetPixels() );
                        RID rid = state.gfx->upload_image( image );
                        tex->SetTexID( reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( rid.value ) ) );
                        tex->BackendUserData = reinterpret_cast<void*>( static_cast<uintptr_t>( rid.value ) );
                        tex->SetStatus( ImTextureStatus_OK );
                        NC_LOG_TRACE_C(
                            log::GUI, "Handle texture lifecycle: tex={}, status=WantCreate",
                            reinterpret_cast<void*>( tex )
                        );
                        break;
                    }
                    case ImTextureStatus_WantDestroy: {
                        RID rid( reinterpret_cast<uintptr_t>( tex->BackendUserData ) );
                        state.gfx->destroy_resource( rid );
                        tex->BackendUserData = nullptr;
                        tex->SetTexID( ImTextureID_Invalid );
                        tex->SetStatus( ImTextureStatus_Destroyed );
                        NC_LOG_TRACE_C(
                            log::GUI, "Handle texture lifecycle: tex={}, status=WantDestroy",
                            reinterpret_cast<void*>( tex )
                        );
                        break;
                    }
                    case ImTextureStatus_WantUpdates: {
                        RID old_rid( reinterpret_cast<uintptr_t>( tex->BackendUserData ) );
                        state.gfx->destroy_resource( old_rid );
                        Image image( tex->Width, tex->Height, tex->GetPixels() );
                        RID new_rid = state.gfx->upload_image( image );
                        tex->SetTexID( reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( new_rid.value ) ) );
                        tex->BackendUserData = reinterpret_cast<void*>( static_cast<uintptr_t>( new_rid.value ) );
                        tex->SetStatus( ImTextureStatus_OK );
                        NC_LOG_TRACE_C(
                            log::GUI, "Handle texture lifecycle: tex={}, status=WantUpdates",
                            reinterpret_cast<void*>( tex )
                        );
                        break;
                    }
                    case ImTextureStatus_OK:
                    case ImTextureStatus_Destroyed:
                        break;
                }
            };

            // Handle texture lifecycle
            if (dd->Textures) {
                for (auto tex : *dd->Textures) {
                    handle_tex( tex );
                }
            }

            for (int i = 0; i < dd->CmdListsCount; i++) {
                const ImDrawList* draw_list = dd->CmdLists[i];

                for (int j = 0; j < draw_list->CmdBuffer.Size; j++) {
                    const ImDrawCmd& cmd = draw_list->CmdBuffer[j];
                    if (cmd.UserCallback) {
                        cmd.UserCallback( draw_list, &cmd );
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

                    RID tex_rid( reinterpret_cast<uintptr_t>( cmd.GetTexID() ) );
                    state.gfx->draw_indexed(
                        draw_list->VtxBuffer.Data + cmd.VtxOffset, draw_list->VtxBuffer.Size - cmd.VtxOffset,
                        draw_list->IdxBuffer.Data + cmd.IdxOffset, cmd.ElemCount, tex_rid, clip_rect
                    );
                }
            }
        } );
}

} // namespace nc
