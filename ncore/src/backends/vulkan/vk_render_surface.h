#pragma once

#include <DeviceContext.h>
#include <EngineFactoryVk.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <SwapChain.h>
#include <Texture.h>

#include <ncore/modules/video/render_surface.h>

namespace nc {

class VkRenderSurface : public IRenderSurface {
    NCLASS( VkRenderSurface, IRenderSurface )

    template<typename T>
    using DiligentRef = Diligent::RefCntAutoPtr<T>;

public:
    VkRenderSurface(
        void* native_handle, Vec2 size, Diligent::IEngineFactoryVk* factory, Diligent::IRenderDevice* device,
        Diligent::IDeviceContext* ctx
    );

    void begin_frame() override;
    void end_frame() override;
    void set_clear_color( Color color ) override;
    void set_vsync( bool enabled ) override;
    void set_render_size( Vec2 size ) override;
    Vec2 get_surface_size() const override;

    Diligent::ITextureView* get_current_rtv() const
    {
        return swap_chain ? swap_chain->GetCurrentBackBufferRTV() : nullptr;
    }

    Diligent::ITextureView* get_depth_dsv() const
    {
        return swap_chain ? swap_chain->GetDepthBufferDSV() : nullptr;
    }

private:
    DiligentRef<Diligent::ISwapChain> swap_chain;
    Diligent::IDeviceContext* device_ctx;

    float clear_color[4] = { 0.35f, 0.35f, 0.35f, 1.0f };
    bool vsync_enabled   = true;
};

} // namespace nc
