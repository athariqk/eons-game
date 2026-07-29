#pragma once

#include <BasicMath.hpp>
#include <DurationQueryHelper.hpp>
#include <EngineFactoryVk.h>
#include <PipelineState.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <Sampler.h>
#include <ScopedQueryHelper.hpp>
#include <Texture.h>

#include <ncore/modules/video/rhi.h>

namespace nc {

class DiligentRHI : public IRHI {
    NCLASS( DiligentRHI, IRHI )

public:
    DiligentRHI();
    ~DiligentRHI() override;

    DiligentRHI( const DiligentRHI& )           = delete;
    DiligentRHI& operator=( const DiligentRHI ) = delete;

    RID create_deferred_context( GpuQueue queue ) override;
    void set_context_state( bool deferred, RID deferred_id = 0 ) override;

    void set_queue( GpuQueue queue ) override;
    void sync_queue( GpuQueue queue ) override;

    RID swapchain_create( const SwapChainDesc& desc ) override;
    Vec2 swapchain_get_size( RID sc ) override;
    void swapchain_set_size( RID sc, Vec2 size ) override;
    void swapchain_present( RID sc, bool sync_interval ) override;
    void* swapchain_get_view( RID sc, TextureViewType view ) override;
    void swapchain_destroy( RID sc ) override;

    RID gfx_pipeline_create( const GraphicsPSODesc& desc, DynArray<RID> resource_signatures = {} ) override;
    void gfx_pipeline_bind( RID pipeline ) override;
    void gfx_pipeline_reload( RID pipeline ) override;

    void render_target_bind( std::span<const void*> rtvs, void* dsv = nullptr ) override;
    void render_target_set_viewport( std::span<const Viewport> viewports ) override;
    void render_target_set_scissor_rect( std::span<const Rect> rect ) override;
    void render_target_clear_color( void* rtv, const Color& color ) override;
    void render_target_clear_depth( void* dsv, float depth = 1.0f, uint8_t stencil = 0 ) override;

    void commands_record_begin() override;
    void* commands_record_end() override;
    void commands_record_execute( void* p_cmd_list ) override;
    void commands_release() override;

    void compute_pipeline_create() override;

    RID texture_create( const TextureDesc& desc ) override;
    void texture_binding_update( RID texture, RID binding, const char* name ) override;
    void* texture_get_view( RID texture, TextureViewType view ) override;

    RID sampler_create( const SamplerDesc& desc ) override;
    void sampler_update_binding( RID sampler, RID binding, const char* name ) override;

    RID buffer_create( const BufferDesc& desc ) override;
    void buffer_update( RID buffer, const void* data, size_t size ) override;
    void buffer_update_binding( RID buffer, RID binding, const char* name ) override;
    void
    vertex_buffers_bind( std::span<const RID> buffers, uint32_t slot, std::span<const uint64_t> offsets = {} ) override;
    void index_buffer_bind( RID buffer, uint32_t offset ) override;

    RID resource_signature_create( const ResourceSignatureDesc& desc ) override;

    RID resource_binding_create( RID signature ) override;
    void resource_binding_commit( RID binding ) override;

    bool is_rid_owned( RID rid ) override;
    void destroy_resource( RID rid ) override;

    void draw( uint32_t vertex_count, uint32_t start_vertex = 0, uint32_t instance_count = 1 ) override;
    void draw_indexed(
        uint32_t index_count, uint32_t start_index = 0, int32_t base_vertex = 0, uint32_t instance_count = 1
    ) override;

    void load_pso_cache() override;
    void save_pso_cache() override;

    void begin_queries() override;
    void end_queries() override;
    Stats get_stats() override;

private:
    Diligent::IDeviceContext* get_active_ctx_();
    Diligent::IDeviceContext* get_imm_ctx_();

    Diligent::RefCntAutoPtr<Diligent::IEngineFactoryVk> engine_factory;
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device;

    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> ctx_gfx;  // immediate graphics context
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> ctx_comp; // immediate compute context
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> ctx_tx;   // immediate transfer context

    ResourcePool<Diligent::RefCntAutoPtr<Diligent::IDeviceContext>> ctx_gfx_defer;
    ResourcePool<Diligent::RefCntAutoPtr<Diligent::IDeviceContext>> ctx_comp_defer;
    ResourcePool<Diligent::RefCntAutoPtr<Diligent::IDeviceContext>> ctx_tx_defer;

    Diligent::RefCntAutoPtr<Diligent::ICommandList> cmd_list;

    ResourcePool<Diligent::RefCntAutoPtr<Diligent::ISwapChain>> swapchains;
    ResourcePool<Diligent::RefCntAutoPtr<Diligent::IPipelineState>> pipelines;
    ResourcePool<Diligent::RefCntAutoPtr<Diligent::ITexture>> textures;
    ResourcePool<Diligent::RefCntAutoPtr<Diligent::IBuffer>> buffers;
    ResourcePool<Diligent::RefCntAutoPtr<Diligent::ISampler>> samplers;

    ResourcePool<Diligent::RefCntAutoPtr<Diligent::IPipelineResourceSignature>> res_signatures;
    ResourcePool<Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding>> res_bindings;

    Diligent::RefCntAutoPtr<Diligent::IVertexPool> vertex_pool;

    std::unique_ptr<Diligent::ScopedQueryHelper> pipeline_stats_query;
    std::unique_ptr<Diligent::ScopedQueryHelper> occlusion_query;
    std::unique_ptr<Diligent::ScopedQueryHelper> duration_query;
    std::unique_ptr<Diligent::DurationQueryHelper> duration_from_timestamps_query;
    Diligent::QueryDataPipelineStatistics pipeline_stats_data;
    Diligent::QueryDataOcclusion occlusion_data;
    Diligent::QueryDataDuration duration_data;
    double duration_from_timestamps = 0;

    GpuQueue active_queue  = GpuQueue::Graphics;
    bool is_deferred       = false;
    RID active_deferred_id = 0;
};

} // namespace nc
