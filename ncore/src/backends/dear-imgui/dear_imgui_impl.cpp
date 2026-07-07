#include "dear_imgui_impl.h"

#include <imgui.h>

#include <ncore/modules/events/input_event.h>
#include <ncore/modules/video/graphics_module.h>
#include <ncore/modules/video/resources/image.h>
#include <ncore/modules/video/window_module.h>

namespace nc {

DearImGuiImpl::DearImGuiImpl( GraphicsModule* p_gfx, IWindowModule* p_windows ) : gfx( p_gfx ), windows( p_windows ) {}

DearImGuiImpl::~DearImGuiImpl() {}

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

Error DearImGuiImpl::init()
{
    IMGUI_CHECKVERSION();
    imgui_ctx = ImGui::CreateContext();
    ImGui::StyleColorsClassic();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;
    auto size        = gfx->get_surface_size();
    io.DisplaySize.x = size.X;
    io.DisplaySize.y = size.Y;

    for (int i = 0; i < ImGuiMouseCursor_COUNT; i++) {
        auto imgui_cursor        = static_cast<ImGuiMouseCursor>( i );
        auto cursor_type         = map_cursor_type( imgui_cursor );
        cursor_map[imgui_cursor] = cursor_type;
    }

    return Error::OK;
}

void DearImGuiImpl::finalize()
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

void DearImGuiImpl::begin_frame()
{
    ImGuiIO& io      = ImGui::GetIO();
    auto size        = gfx->get_surface_size();
    io.DisplaySize.x = size.X;
    io.DisplaySize.y = size.Y;
    ImGui::NewFrame();
}

void DearImGuiImpl::render_frame()
{
    ImGui::Render();

    // sets the wanted cursor type, it should be safe to subscript directly
    // since we already done exhaustive mapping in init
    auto wanted_cursor = cursor_map[ImGui::GetMouseCursor()];
    windows->set_cursor_type( wanted_cursor );

    ImDrawData* dd    = ImGui::GetDrawData();
    Vec2 display_size = Vec2( dd->DisplaySize.x, dd->DisplaySize.y );
    if (display_size.is_zero())
        return;

    auto handle_tex = [this]( ImTextureData* tex ) {
        switch (tex->Status) {
            case ImTextureStatus_WantCreate: {
                Image image( tex->Width, tex->Height, tex->GetPixels() );
                RID rid = gfx->upload_image( image );
                tex->SetTexID( reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( rid.value ) ) );
                tex->BackendUserData = reinterpret_cast<void*>( static_cast<uintptr_t>( rid.value ) );
                tex->SetStatus( ImTextureStatus_OK );
                NC_LOG_TRACE_C(
                    log::GUI, "DearImGuiImpl::render_frame() - Handle texture lifecycle: tex={}, status=WantCreate",
                    reinterpret_cast<void*>( tex )
                );
                break;
            }
            case ImTextureStatus_WantDestroy: {
                RID rid( reinterpret_cast<uintptr_t>( tex->BackendUserData ) );
                gfx->destroy_resource( rid );
                tex->BackendUserData = nullptr;
                tex->SetTexID( ImTextureID_Invalid );
                tex->SetStatus( ImTextureStatus_Destroyed );
                NC_LOG_TRACE_C(
                    log::GUI, "DearImGuiImpl::render_frame() - Handle texture lifecycle: tex={}, status=WantDestroy",
                    reinterpret_cast<void*>( tex )
                );
                break;
            }
            case ImTextureStatus_WantUpdates: {
                RID old_rid( reinterpret_cast<uintptr_t>( tex->BackendUserData ) );
                gfx->destroy_resource( old_rid );
                Image image( tex->Width, tex->Height, tex->GetPixels() );
                RID new_rid = gfx->upload_image( image );
                tex->SetTexID( reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( new_rid.value ) ) );
                tex->BackendUserData = reinterpret_cast<void*>( static_cast<uintptr_t>( new_rid.value ) );
                tex->SetStatus( ImTextureStatus_OK );
                NC_LOG_TRACE_C(
                    log::GUI, "DearImGuiImpl::render_frame() - Handle texture lifecycle: tex={}, status=WantUpdates",
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
            gfx->draw_indexed(
                draw_list->VtxBuffer.Data + cmd.VtxOffset, draw_list->VtxBuffer.Size - cmd.VtxOffset,
                draw_list->IdxBuffer.Data + cmd.IdxOffset, cmd.ElemCount, tex_rid, clip_rect
            );
        }
    }
}

inline static ImGuiKey KeyToImGuiKey( KeyboardEvent::Key key )
{
    switch (key) {
        case KeyboardEvent::Key::W:
            return ImGuiKey_W;
        case KeyboardEvent::Key::A:
            return ImGuiKey_A;
        case KeyboardEvent::Key::S:
            return ImGuiKey_S;
        case KeyboardEvent::Key::D:
            return ImGuiKey_D;
        case KeyboardEvent::Key::UP:
            return ImGuiKey_UpArrow;
        case KeyboardEvent::Key::DOWN:
            return ImGuiKey_DownArrow;
        case KeyboardEvent::Key::LEFT:
            return ImGuiKey_LeftArrow;
        case KeyboardEvent::Key::RIGHT:
            return ImGuiKey_RightArrow;
        case KeyboardEvent::Key::SPACE:
            return ImGuiKey_Space;
        case KeyboardEvent::Key::ENTER:
            return ImGuiKey_Enter;
        case KeyboardEvent::Key::ESC:
            return ImGuiKey_Escape;
        case KeyboardEvent::Key::SHIFT:
            return ImGuiKey_LeftShift;
        case KeyboardEvent::Key::CTRL:
            return ImGuiKey_LeftCtrl;
        case KeyboardEvent::Key::ALT:
            return ImGuiKey_LeftAlt;
        case KeyboardEvent::Key::TAB:
            return ImGuiKey_Tab;
        case KeyboardEvent::Key::BKSP:
            return ImGuiKey_Backspace;
        case KeyboardEvent::Key::UNKNOWN:
            return ImGuiKey_None;
    }
}

bool DearImGuiImpl::process_event( Event* event )
{
    if (!event)
        return false;

    ImGuiIO& io = ImGui::GetIO();

    switch (event->get_type()) {
        case EventType::MOUSE_MOTION: {
            auto* e = static_cast<MouseMotionEvent*>( event );
            io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
            io.AddMousePosEvent( e->position.X, e->position.Y );
            return true;
        }
        case EventType::MOUSE_WHEEL: {
            auto* e = static_cast<MouseWheelEvent*>( event );
            io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
            io.AddMouseWheelEvent( e->scroll_x, e->scroll_y );
            return true;
        }
        case EventType::MOUSE_BUTTON: {
            auto* e    = static_cast<MouseButtonEvent*>( event );
            int button = -1;
            if (e->button == ButtonIndex::LEFT)
                button = 0;
            if (e->button == ButtonIndex::RIGHT)
                button = 1;
            if (e->button == ButtonIndex::MIDDLE)
                button = 2;
            if (button == -1)
                break;
            io.AddMouseSourceEvent( ImGuiMouseSource_Mouse );
            io.AddMouseButtonEvent( button, e->action == ButtonAction::PRESS );
            return true;
        }
        case EventType::TEXT_INPUT: {
            auto* e = static_cast<TextInputEvent*>( event );
            io.AddInputCharactersUTF8( e->text.c_str() );
            return true;
        }
        case EventType::KEYBOARD: {
            auto* e      = static_cast<KeyboardEvent*>( event );
            bool down    = e->action == ButtonAction::PRESS;
            ImGuiKey key = KeyToImGuiKey( e->key );
            if (key != ImGuiKey_None) {
                io.AddKeyEvent( key, down );
                io.SetKeyEventNativeData( key, static_cast<int>( e->key ), 0, static_cast<int>( e->key ) );
            }
            return true;
        }
        case EventType::WINDOW_MOUSE_ENTER:
        case EventType::WINDOW_MOUSE_LEAVE:
            return true;
        case EventType::WINDOW_FOCUS: {
            auto* e = static_cast<WindowFocusEvent*>( event );
            io.AddFocusEvent( e->focused );
            return true;
        }
        case EventType::WINDOW_RESIZE: {
            auto* e        = static_cast<WindowResizeEvent*>( event );
            io.DisplaySize = ImVec2( static_cast<float>( e->width ), static_cast<float>( e->height ) );
            return false;
        }
        case EventType::UNKNOWN:
        case EventType::WINDOW_CLOSE:
        case EventType::WORLD_CHANGE_REQUEST:
        case EventType::WORLD_CHANGE_COMPLETE:
        case EventType::COUNT:
            break;
    }
    return false;
}

} // namespace nc
