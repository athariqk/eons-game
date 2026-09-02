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

#include <ncore/services/video/rhi.h>

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

    RID swapchain_create( const rhi::SwapChainDesc& desc ) override;
    Vec2i swapchain_get_size( RID sc ) override;
    void swapchain_set_size( RID sc, Vec2i size ) override;
    void swapchain_present( RID sc, bool sync_interval ) override;
    void* swapchain_get_view( RID sc, rhi::TextureViewType view ) override;
    void swapchain_destroy( RID sc ) override;

    RID shader_create( const rhi::ShaderCreateDesc& desc ) override;

    RID gfx_pipeline_create( const rhi::GraphicsPSODesc& desc ) override;
    void gfx_pipeline_bind( RID pipeline ) override;
    void gfx_pipeline_reload( RID pipeline ) override;

    void render_target_bind( Span<const void*> rtvs, void* dsv = nullptr ) override;
    void render_target_set_viewport( Span<const Viewport> viewports ) override;
    void render_target_set_scissor_rect( Span<const Rect2i> rect ) override;
    void render_target_clear_color( void* rtv, const Color& color ) override;
    void render_target_clear_depth( void* dsv, float depth = 1.0f, uint8_t stencil = 0 ) override;

    void commands_record_begin() override;
    void* commands_record_end() override;
    void commands_record_execute( void* p_cmd_list ) override;
    void commands_release() override;

    RID compute_pipeline_create( const rhi::ComputePSODesc& desc ) override;
    void compute_pipeline_bind( RID pipeline ) override;
    void compute_dispatch( uint32_t x, uint32_t y, uint32_t z ) override;

    RID texture_create( const rhi::TextureDesc& desc ) override;
    void* texture_view_get( RID texture, rhi::TextureViewType view ) override;
    void texture_binding_update(
        RID p_texture, RID p_binding, rhi::ShaderStage p_shader_type, rhi::TextureViewType p_view_type,
        const char* p_name
    ) override;
    void texture_blit( RID texture_src, RID texture_dest, bool to_swapchain ) override;

    RID sampler_create( const rhi::SamplerDesc& desc ) override;
    void sampler_binding_update( RID p_sampler, RID p_binding, const char* p_name ) override;

    RID buffer_create( const rhi::BufferDesc& desc ) override;
    void* buffer_view_get( RID buffer, rhi::BufferViewType view ) override;
    void buffer_data_write( RID p_buffer, Span<const std::byte> p_src ) override;
    void buffer_data_read( RID p_buffer, Span<std::byte> p_dst ) override;
    void buffer_blit( RID p_src_buffer, RID p_dst_buffer ) override;
    void buffer_binding_update(
        RID p_buffer, RID p_binding, rhi::ShaderStage p_shader_type, rhi::BufferViewType p_view_type, const char* p_name
    ) override;
    void buffer_vertices_bind( Span<const RID> buffers, uint32_t slot, Span<const uint64_t> offsets = {} ) override;
    void buffer_index_bind( RID buffer, uint32_t offset ) override;

    RID resource_signature_create( const rhi::ResourceSignatureDesc& desc ) override;

    /**
     * @brief Create a shader resource mapping.
     */
    RID resource_mapping_create( Span<const rhi::ResourceMappingEntry> p_entries ) override;
    void
    resource_mapping_add_entry( RID p_mapping, const rhi::ResourceMappingEntry& p_entry, bool p_is_unique ) override;

    RID resource_binding_create( RID p_resource_signature ) override;
    void resource_binding_update(
        RID p_resource_binding, RID p_resource_mapping, rhi::ShaderStage p_shader_stages
    ) override;
    void resource_binding_commit( RID resource_binding ) override;

    bool is_rid_owned( RID rid ) override;
    bool destroy_rid( RID rid ) override;

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
    Diligent::IDeviceObject* map_resource_bind_( RID p_resource, rhi::ResourceType p_kind );

    Diligent::RefCntAutoPtr<Diligent::IEngineFactoryVk> engine_factory;
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device;

    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> ctx_gfx;  // immediate graphics context
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> ctx_comp; // immediate compute context
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> ctx_tx;   // immediate transfer context

    Diligent::RefCntAutoPtr<Diligent::ICommandList> cmd_list;

    RIDPool<Diligent::RefCntAutoPtr<Diligent::IDeviceContext>> ctx_gfx_defer;
    RIDPool<Diligent::RefCntAutoPtr<Diligent::IDeviceContext>> ctx_comp_defer;
    RIDPool<Diligent::RefCntAutoPtr<Diligent::IDeviceContext>> ctx_tx_defer;
    RIDPool<Diligent::RefCntAutoPtr<Diligent::ISwapChain>> swapchains;
    RIDPool<Diligent::RefCntAutoPtr<Diligent::IShader>> shaders;
    RIDPool<Diligent::RefCntAutoPtr<Diligent::IPipelineState>> pipelines;
    RIDPool<Diligent::RefCntAutoPtr<Diligent::ITexture>> textures;
    RIDPool<Diligent::RefCntAutoPtr<Diligent::IBuffer>> buffers;
    RIDPool<Diligent::RefCntAutoPtr<Diligent::ISampler>> samplers;
    RIDPool<Diligent::RefCntAutoPtr<Diligent::IPipelineResourceSignature>> res_signatures;
    RIDPool<Diligent::RefCntAutoPtr<Diligent::IResourceMapping>> res_mappings;
    RIDPool<Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding>> res_bindings;

    Diligent::RefCntAutoPtr<Diligent::IVertexPool> vertex_pool;

    Ptr<Diligent::ScopedQueryHelper> pipeline_stats_query;
    Ptr<Diligent::ScopedQueryHelper> occlusion_query;
    Ptr<Diligent::ScopedQueryHelper> duration_query;
    Ptr<Diligent::DurationQueryHelper> duration_from_timestamps_query;
    Diligent::QueryDataPipelineStatistics pipeline_stats_data;
    Diligent::QueryDataOcclusion occlusion_data;
    Diligent::QueryDataDuration duration_data;
    double duration_from_timestamps = 0;

    GpuQueue active_queue  = GpuQueue::GRAPHICS;
    bool is_deferred       = false;
    RID active_deferred_id = 0;
};

} // namespace nc
