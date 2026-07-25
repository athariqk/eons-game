#pragma once

#include <ncore/modules/video/renderer/geometry.h>
#include <ncore/utils/log.h>

namespace nc {

class ShaderCompiler;

/**
 * @brief SDLRenderDevice is a minimal stub of IRenderer.
 *
 * The SDL render path is currently dormant (the application uses the Vulkan
 * backend). This implementation compiles and satisfies the interface but does
 * not perform real rendering. Multi-surface support is not implemented.
 */
class SDLRenderDevice : public IRenderer {
    NCLASS( SDLRenderDevice, IRenderer )

public:
    SDLRenderDevice()           = default;
    ~SDLRenderDevice() override = default;

    Ref<IRenderTarget> create_target( void*, Vec2 ) override
    {
        NC_LOG_ERROR_C( log::GRAPHICS, "SDLRenderDevice::create_target is not implemented" );
        return nullptr;
    }

    RID texture_create( const TextureDesc& ) override
    {
        return RID();
    }
    RID gfx_pipeline_create( const GraphicsPSODesc& ) override
    {
        return RID();
    }
    RID buffer_create( const BufferDesc& ) override
    {
        return RID();
    }

    void gfx_pipeline_bind( RID ) override {}
    void gfx_pipeline_reload( RID ) override {}

    void buffer_update( RID, const void*, size_t ) override {}

    RID resource_binding_create( RID, std::span<const ResourceBindingSlot> ) override
    {
        return RID();
    }
    void resource_binding_commit( RID ) override {}
    void resource_binding_update( RID, std::span<const ResourceBindingSlot> ) override {}

    bool is_rid_owned( RID ) override
    {
        return false;
    }
    void destroy_resource( RID ) override {}

    void* get_native_texture_view( RID ) override
    {
        return nullptr;
    }
    void* get_native_handle() const override
    {
        return nullptr;
    }
    RID get_white_texture() const override
    {
        return RID();
    }

    void render_2d( IRenderTarget& ) override {}
    void render_3d( IRenderTarget& ) override {}

    void draw_quad( Vec4, Vec4, Color ) override {}
    void draw_2d_triangles( std::span<const Vertex2D>, std::span<const uint32_t>, Vec4 ) override {}

#if !defined( NC_DIST )
    void force_reload_pipelines( ShaderCompiler& ) override {}
#endif
};

} // namespace nc
