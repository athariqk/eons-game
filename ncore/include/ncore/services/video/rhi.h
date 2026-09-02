#pragma once

#include <ncore/core/color.h>
#include <ncore/core/object.h>
#include <ncore/core/rect.h>

#include "rhi_types.h"

namespace nc {

enum class GpuQueue : uint8_t {
    GRAPHICS = 0,
    COMPUTE  = 1,
    TRANSFER = 2,
};
NENUM(
    GpuQueue, NENUM_ELEMENT( GpuQueue, GRAPHICS ), NENUM_ELEMENT( GpuQueue, COMPUTE ),
    NENUM_ELEMENT( GpuQueue, TRANSFER )
)

/**
 * @brief IRHI (Render Hardware Interface) defines the implementation contract for Graphics APIs.
 */
class IRHI : public NcObject {
    NCLASS( IRHI, NcObject )

public:
    virtual RID create_deferred_context( GpuQueue queue )                = 0;
    virtual void set_context_state( bool deferred, RID deferred_id = 0 ) = 0;

    /**
     * @brief Set the device context's working queue.
     */
    virtual void set_queue( GpuQueue queue ) = 0;
    /**
     * @brief Waits for specified queue (GPU) to finish what they're doing before moving on.
     */
    virtual void sync_queue( GpuQueue queue ) = 0;

    // Graphics pipeline

    /**
     * @brief Creates a per-window presentation target (swap chain).
     */
    virtual RID swapchain_create( const rhi::SwapChainDesc& desc )               = 0;
    virtual Vec2i swapchain_get_size( RID swapchain )                            = 0;
    virtual void swapchain_set_size( RID swapchain, Vec2i size )                 = 0;
    virtual void swapchain_present( RID swapchain, bool sync_interval )          = 0;
    virtual void* swapchain_get_view( RID swapchain, rhi::TextureViewType view ) = 0;
    virtual void swapchain_destroy( RID swapchain )                              = 0;

    virtual RID shader_create( const rhi::ShaderCreateDesc& desc ) = 0;

    /**
     * @brief Create a graphics pipeline state object.
     * @return Its RID handle.
     */
    virtual RID gfx_pipeline_create( const rhi::GraphicsPSODesc& desc ) = 0;
    /**
     * @brief Bind a graphics PSO to current context.
     */
    virtual void gfx_pipeline_bind( RID pipeline )   = 0;
    virtual void gfx_pipeline_reload( RID pipeline ) = 0;

    struct Viewport {
        Rect2f rect;
        float min_depth = 0;
        float max_depth = 1;
    };

    /**
     * @brief Binds one or more render targets and the depth-stencil buffer to the context.
     */
    virtual void render_target_bind( Span<const void*> render_target_views, void* depth_stencil_view = nullptr ) = 0;
    /**
     * @brief Set viewport(s) of currently bound render target.
     */
    virtual void render_target_set_viewport( Span<const Viewport> viewports ) = 0;
    /**
     * @brief Sets active scissor rects for currently bound render target.
     */
    virtual void render_target_set_scissor_rect( Span<const Rect2i> rect )                                      = 0;
    virtual void render_target_clear_color( void* render_target_view, const Color& color )                      = 0;
    virtual void render_target_clear_depth( void* depth_stencil_view, float depth = 1.0f, uint8_t stencil = 0 ) = 0;

    virtual void commands_record_begin()                     = 0;
    virtual void* commands_record_end()                      = 0;
    virtual void commands_record_execute( void* p_cmd_list ) = 0;
    virtual void commands_release()                          = 0;

    // General compute pipeline

    virtual RID compute_pipeline_create( const rhi::ComputePSODesc& desc ) = 0;
    virtual void compute_pipeline_bind( RID pipeline )                     = 0;
    /**
     * @brief Executes a dispatch compute command.
     * @param x The number of thread groups dispatch in X direction.
     * @param y The number of thread groups dispatch in Y direction.
     * @param z The number of thread groups dispatch in Z direction.
     */
    virtual void compute_dispatch( uint32_t x, uint32_t y, uint32_t z ) = 0;

    // Resources

    virtual RID texture_create( const rhi::TextureDesc& desc )               = 0;
    virtual void* texture_view_get( RID texture, rhi::TextureViewType view ) = 0;
    virtual void texture_binding_update(
        RID p_texture, RID p_binding, rhi::ShaderStage p_shader_type, rhi::TextureViewType p_view_type,
        const char* p_name
    ) = 0;
    /**
     * @brief Copy data from one texture to another.
     * @param texture_src RID handle of source tex.
     * @param texture_dest RID handle of destination tex.
     * @param to_swapchain If true, texture_dest will be interpreted as a swapchain backbuffer RTV texture.
     */
    virtual void texture_blit( RID texture_src, RID texture_dest, bool to_swapchain = false ) = 0;

    virtual RID sampler_create( const rhi::SamplerDesc& desc )                        = 0;
    virtual void sampler_binding_update( RID sampler, RID binding, const char* name ) = 0;

    virtual RID buffer_create( const rhi::BufferDesc& desc )              = 0;
    virtual void* buffer_view_get( RID buffer, rhi::BufferViewType view ) = 0;
    /**
     * @brief Change what's inside an existing buffer.
     * @param p_buffer The existing GPU buffer.
     * @param p_src Source data to copy into the buffer.
     */
    virtual void buffer_data_write( RID p_buffer, Span<const std::byte> p_src ) = 0;
    /**
     * @brief Read data from an existing buffer.
     */
    virtual void buffer_data_read( RID p_buffer, Span<std::byte> p_dst ) = 0;
    /**
     * @brief Copy data from one buffer to another.
     * @param p_src_buffer RID handle of source buffer.
     * @param p_dst_buffer RID handle of destination buffer.
     */
    virtual void buffer_blit( RID p_src_buffer, RID p_dst_buffer ) = 0;
    /**
     * @brief Change which buffer a shader resource variable is currently pointing at.
     */
    virtual void buffer_binding_update(
        RID p_buffer, RID p_binding, rhi::ShaderStage p_shader_type, rhi::BufferViewType p_view_type, const char* p_name
    )                                                                                                              = 0;
    virtual void buffer_vertices_bind( Span<const RID> buffers, uint32_t slot, Span<const uint64_t> offsets = {} ) = 0;
    virtual void buffer_index_bind( RID buffer, uint32_t offset )                                                  = 0;

    /**
     * @brief Create a descriptor set.
     */
    virtual RID resource_signature_create( const rhi::ResourceSignatureDesc& desc ) = 0;

    /**
     * @brief Create a shader resource mapping.
     */
    virtual RID resource_mapping_create( Span<const rhi::ResourceMappingEntry> p_entries ) = 0;
    /**
     * @brief Add/replace a resource mapping entry.
     * @param p_is_unique Perform entry uniqueness validation check.
     */
    virtual void
    resource_mapping_add_entry( RID p_mapping, const rhi::ResourceMappingEntry& p_entry, bool p_is_unique ) = 0;

    /**
     * @brief Create shader resource binding (SRB) from a resource signature.
     * @param p_resource_signature Shader resource signature to create binding for.
     */
    virtual RID resource_binding_create( RID p_resource_signature ) = 0;
    /**
     * @brief Bulk update an SRB.
     */
    virtual void
    resource_binding_update( RID p_resource_binding, RID p_resource_mapping, rhi::ShaderStage p_shader_stages ) = 0;
    /**
     * @brief Commits shader resources to the device context.
     *
     * Before a draw or a dispatch compute command can be invoked, all required resources bound to the Shader
     * Resource Binding object and kept in its internal shader resource cache must be committed to the pipeline.
     */
    virtual void resource_binding_commit( RID resource_binding ) = 0;

    virtual bool is_rid_owned( RID rid ) = 0;
    virtual bool destroy_rid( RID rid )  = 0;

    // Drawing

    /**
     * @brief Submit draw call to the GPU.
     */
    virtual void draw( uint32_t vertex_count, uint32_t start_vertex = 0, uint32_t instance_count = 1 ) = 0;
    /**
     * @brief Submit draw call with index info to the GPU.
     */
    virtual void draw_indexed(
        uint32_t index_count, uint32_t start_index = 0, int32_t base_vertex = 0, uint32_t instance_count = 1
    ) = 0;

    // Utilities

    virtual void load_pso_cache() = 0;
    virtual void save_pso_cache() = 0;

    struct Stats {
        uint64_t input_vertices       = 0;
        uint64_t input_primitives     = 0;
        uint64_t vs_invocations       = 0;
        uint64_t gs_invocations       = 0;
        uint64_t ps_invocations       = 0;
        uint64_t clipping_invocations = 0;
        uint64_t clipping_primitives  = 0;

        uint64_t occlusion_samples_passed = 0;

        double gpu_duration_ms = 0.0;
    };

    virtual void begin_queries() = 0;
    virtual void end_queries()   = 0;
    virtual Stats get_stats()    = 0;
};

} // namespace nc
