#pragma once

#include <ncore/kernel/object.h>
#include <ncore/kernel/rid.h>
#include <ncore/kernel/structures.h>

namespace nc {

enum class BufferType {
    Vertex,
    Index,
    Uniform
};

class IRenderBackend : public NcObject {
    NCLASS( IRenderBackend, NcObject )

public:
    virtual Error init()    = 0;
    virtual void finalize() = 0;

    virtual void begin_frame()                  = 0;
    virtual void end_frame()                    = 0;
    virtual void set_clear_color( Color color ) = 0;
    virtual void set_vsync( bool enabled )      = 0;
    virtual void set_render_size( Vec2 size )   = 0;
    virtual Vec2 get_surface_size() const       = 0;

    virtual RID create_texture( uint32_t w, uint32_t h, const void* pixels )                  = 0;
    virtual RID create_pipeline( std::string_view vs_spirv, std::string_view ps_spirv )       = 0;
    virtual RID create_buffer( BufferType type, size_t size, const void* data, bool dynamic ) = 0;
    virtual void destroy_resource( RID rid )                                                  = 0;

    virtual void batch_push_quad( RID texture, Vec4 dest, Vec4 src, Color tint ) = 0;
    virtual void batch_push_indexed(
        const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count, RID texture,
        Vec4 clip_rect
    )                             = 0;
    virtual void batch_2d_flush() = 0;

    virtual void* get_native_texture_view( RID rid ) = 0;
    virtual void* get_native_device() const          = 0;
};

} // namespace nc
