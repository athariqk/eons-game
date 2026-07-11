#include "vk_render_surface.h"

#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

namespace nc {

VkRenderSurface::VkRenderSurface(
    void* native_handle, Vec2 size, Diligent::IEngineFactoryVk* factory, Diligent::IRenderDevice* device,
    Diligent::IDeviceContext* ctx
) : device_ctx( ctx )
{
    Diligent::NativeWindow native;
#if defined( _WIN32 )
    native.hWnd = native_handle;
#else
    NC_ASSERT( false, "Native handle retrieval not implemented for this platform" );
#endif

    Diligent::SwapChainDesc swap_chain_desc;
    swap_chain_desc.Width     = static_cast<Diligent::Uint32>( size.X );
    swap_chain_desc.Height    = static_cast<Diligent::Uint32>( size.Y );
    swap_chain_desc.IsPrimary = true;
    factory->CreateSwapChainVk( device, ctx, swap_chain_desc, native, &swap_chain );
    NC_ASSERT( swap_chain, "Failed to create Vulkan swap chain" );

    NC_LOG_TRACE_C( log::GRAPHICS, "VkRenderSurface created" );
}

void VkRenderSurface::begin_frame()
{
    if (!swap_chain) {
        return;
    }

    Diligent::ITextureView* rtv = swap_chain->GetCurrentBackBufferRTV();
    Diligent::ITextureView* dsv = swap_chain->GetDepthBufferDSV();
    device_ctx->SetRenderTargets( 1, &rtv, dsv, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    device_ctx->ClearRenderTarget( rtv, clear_color, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
    device_ctx->ClearDepthStencil(
        dsv, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
}

void VkRenderSurface::end_frame()
{
    if (swap_chain) {
        swap_chain->Present( static_cast<Diligent::Uint32>( vsync_enabled ) );
    }
}

void VkRenderSurface::set_clear_color( Color color )
{
    clear_color[0] = color.r / 255.0f;
    clear_color[1] = color.g / 255.0f;
    clear_color[2] = color.b / 255.0f;
    clear_color[3] = color.a / 255.0f;
}

void VkRenderSurface::set_vsync( bool enabled )
{
    vsync_enabled = enabled;
}

void VkRenderSurface::set_render_size( Vec2 size )
{
    if (swap_chain) {
        swap_chain->Resize( static_cast<Diligent::Uint32>( size.X ), static_cast<Diligent::Uint32>( size.Y ) );
    }
}

Vec2 VkRenderSurface::get_surface_size() const
{
    if (!swap_chain) {
        return Vec2( 0, 0 );
    }
    auto& desc = swap_chain->GetDesc();
    return Vec2( static_cast<float>( desc.Width ), static_cast<float>( desc.Height ) );
}

} // namespace nc
