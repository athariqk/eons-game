#pragma once

#include <ncore/modules/video/render_device.h>
#include <ncore/utils/log.h>

namespace nc {

/**
 * @brief SDLRenderDevice is a minimal stub of IRenderDevice.
 *
 * The SDL render path is currently dormant (the application uses the Vulkan
 * backend). This implementation compiles and satisfies the interface but does
 * not perform real rendering. Multi-surface support is not implemented.
 */
class SDLRenderDevice : public IRenderDevice {
    NCLASS( SDLRenderDevice, IRenderDevice )

public:
    SDLRenderDevice()           = default;
    ~SDLRenderDevice() override = default;

    std::unique_ptr<IRenderSurface> create_surface( void* native_handle, Vec2 size ) override
    {
        NC_LOG_ERROR_C( log::GRAPHICS, "SDLRenderDevice::create_surface is not implemented" );
        return nullptr;
    }

    RID create_texture( uint32_t, uint32_t, const void* ) override
    {
        return RID();
    }

    RID create_pipeline( std::string_view, std::string_view ) override
    {
        return RID();
    }

    RID create_buffer( BufferType, size_t, const void*, bool ) override
    {
        return RID();
    }

    void destroy_resource( RID ) override {}

    void batch_push_quad( RID, Vec4, Vec4, Color ) override {}
    void batch_push_indexed( const void*, uint32_t, const uint16_t*, uint32_t, RID, Vec4 ) override {}
    void batch_2d_flush( IRenderSurface& ) override {}

    void* get_native_texture_view( RID ) override
    {
        return nullptr;
    }

    void* get_native_device() const override
    {
        return nullptr;
    }

    RID get_white_texture() const override
    {
        return RID();
    }
};

} // namespace nc
