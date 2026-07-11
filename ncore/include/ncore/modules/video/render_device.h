#pragma once

#include <memory>

#include <ncore/kernel/object.h>
#include <ncore/kernel/rid.h>
#include <ncore/kernel/structures.h>

namespace nc {

class IRenderSurface;

enum class BufferType {
    Vertex,
    Index,
    Uniform
};

/**
 * @brief IRenderDevice owns the shared GPU device, its immediate context,
 * the process-wide resource caches, and the shared 2D batch renderer.
 */
class IRenderDevice : public NcObject {
    NCLASS( IRenderDevice, NcObject )

public:
    /**
     * @brief Creates a per-window presentation surface (swap chain).
     *
     * @param native_whnd Platform-native window handle (e.g. HWND).
     * @param size        Initial surface dimensions.
     */
    virtual std::unique_ptr<IRenderSurface> create_surface( void* native_whnd, Vec2 size ) = 0;

    virtual RID create_texture( uint32_t w, uint32_t h, const void* pixels )                  = 0;
    virtual RID create_pipeline( std::string_view vs_spirv, std::string_view ps_spirv )       = 0;
    virtual RID create_buffer( BufferType type, size_t size, const void* data, bool dynamic ) = 0;
    virtual void destroy_resource( RID rid )                                                  = 0;

    virtual void batch_push_quad( RID texture, Vec4 dest, Vec4 src, Color tint ) = 0;
    virtual void batch_push_indexed(
        const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count, RID texture,
        Vec4 clip_rect
    )                                                     = 0;
    virtual void batch_2d_flush( IRenderSurface& target ) = 0;

    virtual void* get_native_texture_view( RID rid ) = 0;
    virtual void* get_native_device() const          = 0;

    virtual RID get_white_texture() const = 0;
};

} // namespace nc
