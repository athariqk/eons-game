#pragma once

#include <SDL3/SDL_render.h>

#include <ncore/modules/video/render_backend.h>

namespace nc {

class IWindowModule;

class SDLRenderBackend : public IRenderBackend {
    NCLASS( SDLRenderBackend, IRenderBackend )

public:
    SDLRenderBackend( IWindowModule* p_windows ) : windows( p_windows ) {}

    Error init() override;
    void finalize() override;

    void begin_frame() override;
    void end_frame() override;
    void set_clear_color( Color color ) override;
    void set_vsync( bool enabled ) override;
    void set_render_size( Vec2 size ) override;
    Vec2 get_surface_size() const override;

    RID create_texture( uint32_t w, uint32_t h, const void* pixels ) override;
    RID create_pipeline( std::string_view vs_spirv, std::string_view ps_spirv ) override;
    RID create_buffer( BufferType type, size_t size, const void* data, bool dynamic ) override;
    void destroy_resource( RID rid ) override;

    void batch_push_quad( RID texture, Vec4 dest, Vec4 src, Color tint ) override;
    void batch_push_indexed(
        const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count, RID texture,
        Vec4 clip_rect
    ) override;
    void batch_2d_flush() override;

    void* get_native_texture_view( RID rid ) override;
    void* get_native_device() const override;

private:
    IWindowModule* windows;
    SDL_Renderer* m_renderer = nullptr;
};

} // namespace nc
