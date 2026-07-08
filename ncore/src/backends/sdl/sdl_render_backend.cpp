#include "sdl_render_backend.h"

#include <ncore/modules/video/window_module.h>
#include <ncore/utils/log.h>

namespace nc {

Error SDLRenderBackend::init()
{
    auto main_win_id = windows->get_main_window_id();
    NC_ASSERT( main_win_id.is_valid(), "Main window RID is invalid!" );
    SDL_Window* window = SDL_GetWindowFromID( static_cast<SDL_WindowID>( main_win_id.value ) );
    if (!window) {
        NC_LOG_ERROR( log::GRAPHICS, "Failed to get SDL window from RID {}: {}", main_win_id.value, SDL_GetError() );
        return Error::FAIL;
    }
    m_renderer = SDL_CreateRenderer( window, nullptr );
    if (m_renderer) {
        NC_LOG_TRACE_C( log::GRAPHICS, "SDL renderer: {}", SDL_GetRendererName( m_renderer ) );
    } else {
        NC_LOG_ERROR_C( log::GRAPHICS, "SDL renderer creation failed!" );
        return Error::FAIL;
    }
    return Error::OK;
}

void SDLRenderBackend::finalize()
{
    if (m_renderer) {
        SDL_DestroyRenderer( m_renderer );
        m_renderer = nullptr;
    }
}

void SDLRenderBackend::begin_frame()
{
    SDL_RenderClear( m_renderer );
}

void SDLRenderBackend::end_frame()
{
    SDL_RenderPresent( m_renderer );
}

void SDLRenderBackend::set_clear_color( Color color )
{
    SDL_SetRenderDrawColor( m_renderer, color.r, color.g, color.b, color.a );
}

void SDLRenderBackend::set_vsync( bool enabled )
{
    SDL_SetRenderVSync( m_renderer, enabled ? 1 : 0 );
}

void SDLRenderBackend::set_render_size( Vec2 size ) {}

Vec2 SDLRenderBackend::get_surface_size() const
{
    int w = 0, h = 0;
    SDL_GetRenderOutputSize( m_renderer, &w, &h );
    return Vec2( static_cast<float>( w ), static_cast<float>( h ) );
}

RID SDLRenderBackend::create_texture( uint32_t w, uint32_t h, const void* pixels )
{
    return RID();
}

RID SDLRenderBackend::create_pipeline( std::string_view vs_spirv, std::string_view ps_spirv )
{
    return RID();
}

RID SDLRenderBackend::create_buffer( BufferType type, size_t size, const void* data, bool dynamic )
{
    return RID();
}

void SDLRenderBackend::destroy_resource( RID rid ) {}

void SDLRenderBackend::batch_push_quad( RID texture, Vec4 dest, Vec4 src, Color tint )
{
    SDL_SetRenderDrawColor( m_renderer, tint.r, tint.g, tint.b, tint.a );
    SDL_FRect r{ dest.X, dest.Y, dest.w, dest.h };
    SDL_RenderFillRect( m_renderer, &r );
}

void SDLRenderBackend::batch_push_indexed(
    const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count, RID texture,
    Vec4 clip_rect
)
{}

void SDLRenderBackend::batch_2d_flush() {}

void* SDLRenderBackend::get_native_texture_view( RID rid )
{
    return nullptr;
}

void* SDLRenderBackend::get_native_device() const
{
    return m_renderer;
}

} // namespace nc
