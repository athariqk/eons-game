#pragma once

#include <DeviceContext.h>
#include <EngineFactoryVk.h>
#include <PipelineState.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <Texture.h>

#include <ncore/kernel/collection.h>
#include <ncore/modules/video/render_device.h>

#include "batch_renderer_2d.h"
#include "diligent_allocator.h"

namespace nc {

class VkRenderSurface;

class VkRenderDevice : public IRenderDevice {
    NCLASS( VkRenderDevice, IRenderDevice )

    template<typename T>
    using DiligentRef = Diligent::RefCntAutoPtr<T>;

    struct PipelineEntry {
        DiligentRef<Diligent::IPipelineState> PipelineStateObj;
        DiligentRef<Diligent::IShaderResourceBinding> ShaderResBinding;
        Diligent::IShaderResourceVariable* TextureVar = nullptr;
    };

public:
    VkRenderDevice();
    ~VkRenderDevice() override;

    std::unique_ptr<IRenderSurface> create_surface( void* native_whnd, Vec2 size ) override;

    RID create_texture( uint32_t w, uint32_t h, const void* pixels ) override;
    RID create_pipeline( std::string_view vs_spirv, std::string_view ps_spirv ) override;
    RID create_buffer( BufferType type, size_t size, const void* data, bool dynamic ) override;
    void destroy_resource( RID rid ) override;

    void batch_push_quad( RID texture, Vec4 dest, Vec4 src, Color tint ) override;
    void batch_push_indexed(
        const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count, RID texture,
        Vec4 clip_rect
    ) override;
    void batch_2d_flush( IRenderSurface& target ) override;

    void* get_native_texture_view( RID rid ) override;
    void* get_native_device() const override;

    RID get_white_texture() const override
    {
        return white_texture_rid;
    }

private:
    void create_2d_pipeline_state_();

    NcoreDiligentAllocator allocator;

    DiligentRef<Diligent::IEngineFactoryVk> engine_factory;
    DiligentRef<Diligent::IRenderDevice> render_device;
    DiligentRef<Diligent::IDeviceContext> device_ctx;
    DiligentRef<Diligent::IShaderSourceInputStreamFactory> shader_src_factory;

    PagedResourcePool<DiligentRef<Diligent::ITexture>> texture_cache;
    PagedResourcePool<PipelineEntry> pipeline_cache;
    PagedResourcePool<DiligentRef<Diligent::IBuffer>> buffer_cache;

    DiligentRef<Diligent::IPipelineState> pso_2d;
    DiligentRef<Diligent::IShaderResourceBinding> srb_2d;
    Diligent::IShaderResourceVariable* texture_var_2d = nullptr;
    DiligentRef<Diligent::IBuffer> constants_2d;

    DiligentRef<Diligent::ITexture> white_texture;
    Diligent::ITextureView* white_tex_view = nullptr;
    RID white_texture_rid;

    std::unique_ptr<BatchRenderer2D> batch2d;
};

} // namespace nc
