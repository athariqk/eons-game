#pragma once

#include <DeviceContext.h>
#include <PipelineState.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <SwapChain.h>
#include <Texture.h>

#include <ncore/kernel/collection.h>
#include <ncore/modules/video/render_backend.h>

#include "batch_renderer_2d.h"

namespace nc {

class IWindowModule;

class VkRenderBackend : public IRenderBackend {
    NCLASS( VkRenderBackend, IRenderBackend )

    template<typename T>
    using DiligentRef = Diligent::RefCntAutoPtr<T>;

    struct PipelineEntry {
        DiligentRef<Diligent::IPipelineState> PipelineStateObj;
        DiligentRef<Diligent::IShaderResourceBinding> ShaderResBinding;
        Diligent::IShaderResourceVariable* TextureVar = nullptr;
    };

public:
    explicit VkRenderBackend( IWindowModule* windows );
    ~VkRenderBackend() override;

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

    void* get_native_texture_view( RID rid ) const override;
    void* get_native_device() const override;

private:
    void create_2d_pipeline_state_();

    IWindowModule* windows;
    DiligentRef<Diligent::IRenderDevice> render_device;
    DiligentRef<Diligent::IDeviceContext> device_ctx;
    DiligentRef<Diligent::ISwapChain> swap_chain;
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

    std::unique_ptr<BatchRenderer2D> batch2d;

    float clear_color[4] = { 0.35f, 0.35f, 0.35f, 1.0f };
    bool vsync_enabled   = true;
};

} // namespace nc
