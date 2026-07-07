#include "sdl_window_impl.h"

#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <backends/sdl/sdl_type_helpers.h>

#include <ncore/kernel/structures.h>
#include <ncore/modules/video/viewport.h>
#include <ncore/modules/video/window_module.h>
#include <ncore/utils/log.h>

namespace nc {

Error SDLWindowImpl::init()
{
    for (int i = 0; i < static_cast<int>( SDL_SYSTEM_CURSOR_COUNT ); ++i) {
        auto cursor_type    = static_cast<CursorType>( i );
        auto sdl_sys_cursor = SDLTypeHelpers::to_sdl_sys_cursor( cursor_type );
        mouse_cursors[cursor_type] =
            cursor_type == CursorType::DEFAULT ? SDL_GetDefaultCursor() : SDL_CreateSystemCursor( sdl_sys_cursor );
    }

    return Error::OK;
}

void SDLWindowImpl::finalize()
{
    int num_windows = 0;
    auto windows    = SDL_GetWindows( &num_windows );
    for (int i = 0; i < num_windows; ++i) {
        SDL_DestroyWindow( windows[i] );
    }
    SDL_free( windows );
    NC_LOG_TRACE_C( log::GRAPHICS, "Destroyed {} SDL window(s)", num_windows );

    for (auto& [cursor_type, cursor] : mouse_cursors) {
        if (cursor == SDL_GetDefaultCursor()) {
            // the map contains multiple refs to the default cursor,
            // so we don't want to destroy it multiple times
            continue;
        }
        if (cursor) {
            SDL_DestroyCursor( cursor );
        }
    }
    mouse_cursors.clear();
}

RID SDLWindowImpl::create_window( const std::string& title, Vec2 size, bool fullscreen )
{
    int flags = 0;
    if (fullscreen)
        flags = SDL_WINDOW_FULLSCREEN;
    auto sdl_win = SDL_CreateWindow(
        title.c_str(), static_cast<int>( size.X ), static_cast<int>( size.Y ), flags | SDL_WINDOW_RESIZABLE
    );
    auto sdl_id = SDL_GetWindowID( sdl_win );
    if (sdl_win) {
        NC_LOG_TRACE_C( log::GRAPHICS, "New SDL window: ({}x{}), ID: {}", size.X, size.Y, sdl_id );
    } else {
        NC_LOG_ERROR_C( log::GRAPHICS, "SDL window creation failed!" );
        return RID();
    }

    SDL_SetWindowPosition( sdl_win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED );
    SDL_ShowWindow( sdl_win );

    auto rid = RID( sdl_id );

    auto vp = std::make_unique<Viewport>( Vec4{ 0, 0, size.X, size.Y } );
    viewport.emplace( rid, std::move( vp ) );

    return rid;
}

void SDLWindowImpl::destroy_window( RID window_id )
{
    SDL_DestroyWindow( SDL_GetWindowFromID( static_cast<SDL_WindowID>( window_id.value ) ) );
}

RID SDLWindowImpl::get_main_window_id() const
{
    int num_windows = 0;
    auto windows    = SDL_GetWindows( &num_windows );
    RID rid         = num_windows > 0 ? RID( SDL_GetWindowID( windows[0] ) ) : RID();
    SDL_free( windows );
    return rid;
}

Vec2 SDLWindowImpl::get_resolution( RID window_id ) const
{
    int width, height;
    auto win = SDL_GetWindowFromID( static_cast<SDL_WindowID>( window_id.value ) );
    SDL_GetWindowSizeInPixels( win, &width, &height );
    return Vec2( static_cast<float>( width ), static_cast<float>( height ) );
}

void SDLWindowImpl::set_title( RID window_id, const std::string& title ) const
{
    auto sdl_win = SDL_GetWindowFromID( static_cast<SDL_WindowID>( window_id.value ) );
    SDL_SetWindowTitle( sdl_win, title.c_str() );
}

void SDLWindowImpl::set_cursor_type( CursorType cursor_type )
{
    auto cursor = mouse_cursors[cursor_type];
    SDL_SetCursor( cursor );
}

bool SDLWindowImpl::show_message_box( MessageBoxType type, const std::string& title, const std::string& message ) const
{
    auto sdl_win              = SDL_GetWindowFromID( static_cast<SDL_WindowID>( get_main_window_id().value ) );
    SDL_MessageBoxFlags flags = 0;
    switch (type) {
        case MessageBoxType::INFO:
            flags = SDL_MESSAGEBOX_INFORMATION;
            break;
        case MessageBoxType::WARNING:
            flags = SDL_MESSAGEBOX_WARNING;
            break;
        case MessageBoxType::ERROR:
            flags = SDL_MESSAGEBOX_ERROR;
            break;
        case MessageBoxType::COUNT:
            break;
    }
    return SDL_ShowSimpleMessageBox( flags, title.c_str(), message.c_str(), sdl_win );
}

Viewport* SDLWindowImpl::get_viewport() const
{
    auto main_window_id = get_main_window_id();
    auto it             = viewport.find( main_window_id );
    return it != viewport.end() ? it->second.get() : nullptr;
}

void* SDLWindowImpl::get_native_handle( RID window_id ) const
{
    auto sdl_win = SDL_GetWindowFromID( static_cast<SDL_WindowID>( window_id.value ) );

    void* native_handle = nullptr;
    if (auto props = SDL_GetWindowProperties( sdl_win )) {
#if defined( _WIN32 )
        native_handle = SDL_GetPointerProperty( props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr );
#else
        NC_ASSERT( false, "Native handle retrieval not implemented for this platform" );
#endif
    }

    return native_handle;
}

} // namespace nc
