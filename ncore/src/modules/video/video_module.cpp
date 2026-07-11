#include <SDL3/SDL_events.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <backends/sdl/sdl_type_helpers.h>

#include <ncore/kernel/collection.h>
#include <ncore/kernel/structures.h>
#include <ncore/modules/video/video_module.h>
#include <ncore/modules/video/viewport.h>
#include <ncore/utils/config.h>
#include <ncore/utils/log.h>

namespace nc {

struct VideoModule::Impl {
    UnorderedMap<uint32_t, std::unique_ptr<Viewport>> viewports;
    UnorderedMap<CursorType, SDL_Cursor*> mouse_cursors;
    Vector<uint32_t> window_ids;

    SDL_Window* get_sdl_window( uint32_t id )
    {
        auto window = SDL_GetWindowFromID( static_cast<SDL_WindowID>( id ) );
        NC_ASSERT_NULL_MSG(
            window, std::format( "Failed to get SDL window with ID {}: {}", id, SDL_GetError() ).c_str()
        );
        return window;
    }

    void force_update_window_ids()
    {
        window_ids.clear();
        int num_windows = 0;
        auto windows    = SDL_GetWindows( &num_windows );
        for (int i = 0; i < num_windows; i++) {
            window_ids.push_back( SDL_GetWindowID( windows[i] ) );
        }
        SDL_free( windows );
    }
};

VideoModule::VideoModule() : pImpl( std::make_unique<Impl>() ) {}

VideoModule::~VideoModule() = default;

Error VideoModule::init( ConfFile& cfg_file )
{
    settings = cfg_file.read<VideoSettings>();

    for (int i = 0; i < static_cast<int>( SDL_SYSTEM_CURSOR_COUNT ); ++i) {
        auto cursor_type    = static_cast<CursorType>( i );
        auto sdl_sys_cursor = SDLTypeHelpers::to_sdl_sys_cursor( cursor_type );
        pImpl->mouse_cursors[cursor_type] =
            cursor_type == CursorType::DEFAULT ? SDL_GetDefaultCursor() : SDL_CreateSystemCursor( sdl_sys_cursor );
    }

    return Error::OK;
}

void VideoModule::finalize()
{
    // for (auto id : pImpl->window_ids) {
    //     SDL_DestroyWindow( SDL_GetWindowFromID( id ) );
    // }
    // pImpl->window_ids.clear();

    for (auto& [cursor_type, cursor] : pImpl->mouse_cursors) {
        if (cursor == SDL_GetDefaultCursor()) {
            continue;
        }
        if (cursor) {
            SDL_DestroyCursor( cursor );
        }
    }
    pImpl->mouse_cursors.clear();
    pImpl->viewports.clear();
}

uint32_t VideoModule::create_window( uint8_t flags )
{
    SDL_WindowFlags sdl_flags = static_cast<SDL_WindowFlags>( 0 );
    if (flags & static_cast<uint8_t>( WindowFlag::RESIZABLE )) {
        sdl_flags |= SDL_WINDOW_RESIZABLE;
    }

    auto window = SDL_CreateWindow( nullptr, 0, 0, sdl_flags | SDL_WINDOW_HIDDEN );
    auto id     = SDL_GetWindowID( window );
    if (window) {
        NC_LOG_TRACE_C( log::GRAPHICS, "New SDL window, ID: {}", id );
    } else {
        NC_LOG_ERROR_C( log::GRAPHICS, "SDL window creation failed!" );
        return uint32_t();
    }

    pImpl->viewports[id] = std::make_unique<Viewport>( Vec4() );

    pImpl->window_ids.push_back( id );

    return static_cast<uint32_t>( id );
}

void VideoModule::set_window_position( uint32_t window_id, Vec2 position ) const
{
    auto window = pImpl->get_sdl_window( window_id );
    if (!SDL_SetWindowPosition( window, static_cast<int>( position.X ), static_cast<int>( position.Y ) )) {
        NC_LOG_ERROR_C( log::GRAPHICS, "Failed to set position for window {}: {}", window_id, SDL_GetError() );
    }
}

void VideoModule::set_window_centered( uint32_t window_id ) const
{
    auto window = pImpl->get_sdl_window( window_id );
    if (!SDL_SetWindowPosition( window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED )) {
        NC_LOG_ERROR_C( log::GRAPHICS, "Failed to set centered for window {}: {}", window_id, SDL_GetError() );
    }
}

void VideoModule::set_window_visible( uint32_t window_id, bool visible ) const
{
    auto window  = pImpl->get_sdl_window( window_id );
    bool success = false;
    if (visible) {
        success = SDL_ShowWindow( window );
    } else {
        success = SDL_HideWindow( window );
    }
    if (!success)
        NC_LOG_ERROR_C( log::GRAPHICS, "Failed to set visibility for window {}: {}", window_id, SDL_GetError() );
}

Vec2 VideoModule::get_window_resolution( uint32_t window_id ) const
{
    int width = 0, height = 0;
    auto window = pImpl->get_sdl_window( window_id );
    if (!SDL_GetWindowSizeInPixels( window, &width, &height )) {
        NC_LOG_ERROR_C( log::GRAPHICS, "Failed to get resolution for window {}: {}", window_id, SDL_GetError() );
    }
    return Vec2( static_cast<float>( width ), static_cast<float>( height ) );
}

void VideoModule::set_window_resolution( uint32_t window_id, Vec2 resolution )
{
    auto window = pImpl->get_sdl_window( window_id );
    pImpl->viewports[window_id]->set_size( resolution.X, resolution.Y );
    if (!SDL_SetWindowSize( window, static_cast<int>( resolution.X ), static_cast<int>( resolution.Y ) )) {
        NC_LOG_ERROR_C( log::GRAPHICS, "Failed to set resolution for window {}: {}", window_id, SDL_GetError() );
    }
}

void VideoModule::set_window_icon( uint32_t window_id, const Image& image ) const {
    // TODO
}

void VideoModule::set_window_title( uint32_t window_id, const std::string& title ) const
{
    auto window = pImpl->get_sdl_window( window_id );
    if (!SDL_SetWindowTitle( window, title.c_str() )) {
        NC_LOG_ERROR_C( log::GRAPHICS, "Failed to set title for window {}: {}", window_id, SDL_GetError() );
    }
}

void VideoModule::set_window_fullscreen( uint32_t window_id, bool fullscreen )
{
    auto window = pImpl->get_sdl_window( window_id );
    if (!SDL_SetWindowFullscreen( window, fullscreen )) {
        NC_LOG_ERROR_C( log::GRAPHICS, "Failed to set fullscreen for window {}: {}", window_id, SDL_GetError() );
    }
}

bool VideoModule::pop_window( uint32_t window_id )
{
    NC_LOG_TRACE_C( log::GRAPHICS, "Destroying SDL window, ID: {}", window_id );
    auto window = pImpl->get_sdl_window( window_id );
    if (!window) {
        return false;
    }
    SDL_DestroyWindow( window );
    pImpl->viewports.erase( window_id );
    auto erased = std::erase( pImpl->window_ids, window_id );
    NC_ASSERT( erased > 0, "Failed removing window id from cache" );
    return true;
}

uint32_t VideoModule::get_main_window_id() const
{
    return pImpl->window_ids.empty() ? -1 : pImpl->window_ids.front();
}

void VideoModule::set_cursor_type( CursorType cursor_type )
{
    auto cursor = pImpl->mouse_cursors[cursor_type];
    if (cursor) {
        SDL_SetCursor( cursor );
    }
}

bool VideoModule::show_message_box( MessageBoxType type, const std::string& title, const std::string& message ) const
{
    auto window               = SDL_GetWindowFromID( static_cast<SDL_WindowID>( get_main_window_id() ) );
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
    return SDL_ShowSimpleMessageBox( flags, title.c_str(), message.c_str(), window );
}

Viewport* VideoModule::get_viewport() const
{
    return get_viewport( get_main_window_id() );
}

Viewport* VideoModule::get_viewport( uint32_t window_id ) const
{
    auto it = pImpl->viewports.find( window_id );
    if (it != pImpl->viewports.end())
        return it->second.get();
    return nullptr;
}

void* VideoModule::get_native_whnd( uint32_t window_id ) const
{
    auto window = pImpl->get_sdl_window( window_id );

    void* native_handle = nullptr;
    if (auto props = SDL_GetWindowProperties( window )) {
#if defined( _WIN32 )
        native_handle = SDL_GetPointerProperty( props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr );
#else
        NC_ASSERT( false, "Native handle retrieval not implemented for this platform" );
#endif
    }

    return native_handle;
}

void VideoModule::pump_events()
{
    event_queue.clear();

    SDL_Event sdl_events[64];
    int count = SDL_PeepEvents( sdl_events, 64, SDL_GETEVENT, SDL_EVENT_WINDOW_FIRST, SDL_EVENT_WINDOW_LAST );

    for (int i = 0; i < count; ++i) {
        auto& e = sdl_events[i];
        switch (e.type) {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                event_queue.push_back( WindowCloseEvent{ e.window.windowID } );
                // Let higher level systems handle it
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                event_queue.push_back( WindowResizeEvent{ e.window.windowID, e.window.data1, e.window.data2 } );
                break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                event_queue.push_back( WindowFocusEvent{ e.window.windowID, true } );
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                event_queue.push_back( WindowFocusEvent{ e.window.windowID, false } );
                break;
            case SDL_EVENT_WINDOW_MOUSE_ENTER:
                event_queue.push_back( WindowMouseEnterEvent{ e.window.windowID } );
                break;
            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                event_queue.push_back( WindowMouseLeaveEvent{ e.window.windowID } );
                break;
            default:
                break;
        }
    }
}

} // namespace nc
