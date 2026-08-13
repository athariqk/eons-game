#pragma once

#include <ncore/core/color.h>
#include <ncore/core/object.h>
#include <ncore/core/rect.h>

#include "rhi_types.h"

namespace nc {

/**
 * @brief IRHI defines a set of APIs for a specific
 * Render Hardware Interface implementation.
 */
class IRHI : public NcObject {
    NCLASS( IRHI, NcObject )

public:
    enum class GpuQueue : uint8_t {
        Graphics = 0,
        Compute  = 1,
        Transfer = 2,
    };

    virtual RID create_deferred_context( GpuQueue queue )                = 0;
    virtual void set_context_state( bool deferred, RID deferred_id = 0 ) = 0;

    virtual void set_queue( GpuQueue queue )  = 0;
    virtual void sync_queue( GpuQueue queue ) = 0;

    // Graphics pipeline

    /**
     * @brief Creates a per-window presentation target (swap chain).
     */
    virtual RID swapchain_create( const SwapChainDesc& desc )        = 0;
    virtual Vec2 swapchain_get_size( RID sc )                        = 0;
    virtual void swapchain_set_size( RID sc, Vec2 size )             = 0;
    virtual void swapchain_present( RID sc, bool sync_interval )     = 0;
    virtual void* swapchain_get_view( RID sc, TextureViewType view ) = 0;
    virtual void swapchain_destroy( RID sc )                         = 0;

    /**
     * @param resource_signatures List of explicitly created resource signature handles.
     */
    virtual RID gfx_pipeline_create( const GraphicsPSODesc& desc, DynamicArray<RID> resource_signatures = {} ) = 0;
    virtual void gfx_pipeline_bind( RID pipeline )                                                             = 0;
    virtual void gfx_pipeline_reload( RID pipeline )                                                           = 0;

    struct Viewport {
        Rect rect;
        float min_depth = 0;
        float max_depth = 1;
    };

    virtual void render_target_bind( Span<const void*> rtvs, void* dsv = nullptr ) = 0;
    /**
     * @brief Set viewport(s) of currently bound render target.
     */
    virtual void render_target_set_viewport( Span<const Viewport> viewports )                    = 0;
    virtual void render_target_set_scissor_rect( Span<const Rect> rect )                         = 0;
    virtual void render_target_clear_color( void* rtv, const Color& color )                      = 0;
    virtual void render_target_clear_depth( void* dsv, float depth = 1.0f, uint8_t stencil = 0 ) = 0;

    virtual void commands_record_begin()                     = 0;
    virtual void* commands_record_end()                      = 0;
    virtual void commands_record_execute( void* p_cmd_list ) = 0;
    virtual void commands_release()                          = 0;

    // General compute pipeline

    virtual RID compute_pipeline_create( const ComputePSODesc& desc ) = 0;
    virtual void compute_pipeline_bind( RID pipeline )                = 0;
    virtual void dispatch( uint32_t x, uint32_t y, uint32_t z )       = 0;

    virtual void texture_compute_update( RID texture, RID binding, const char* name, TextureViewType view ) = 0;
    virtual void buffer_compute_update( RID buffer, RID binding, const char* name )                         = 0;

    // Resources

    virtual RID texture_create( const TextureDesc& desc )                             = 0;
    virtual void texture_binding_update( RID texture, RID binding, const char* name ) = 0;
    virtual void* texture_get_view( RID texture, TextureViewType view )               = 0;

    virtual RID sampler_create( const SamplerDesc& desc )                             = 0;
    virtual void sampler_update_binding( RID sampler, RID binding, const char* name ) = 0;

    virtual RID buffer_create( const BufferDesc& desc )                                                           = 0;
    virtual void buffer_update( RID buffer, const void* data, size_t size )                                       = 0;
    virtual void buffer_update_binding( RID buffer, RID binding, const char* name )                               = 0;
    virtual void vertex_buffers_bind( Span<const RID> buffers, uint32_t slot, Span<const uint64_t> offsets = {} ) = 0;
    virtual void index_buffer_bind( RID buffer, uint32_t offset )                                                 = 0;

    virtual RID resource_signature_create( const ResourceSignatureDesc& desc ) = 0;

    virtual RID resource_binding_create( RID signature ) = 0;
    virtual void resource_binding_commit( RID binding )  = 0;

    virtual bool is_rid_owned( RID rid )     = 0;
    virtual void destroy_resource( RID rid ) = 0;

    // Drawing

    virtual void draw( uint32_t vertex_count, uint32_t start_vertex = 0, uint32_t instance_count = 1 ) = 0;
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
