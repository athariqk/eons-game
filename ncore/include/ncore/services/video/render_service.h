#pragma once

#include <ncore/resources/material_shader.h>
#include <memory>

#include <ncore/core/collection.h>
#include <ncore/core/color.h>
#include <ncore/core/matrix.h>
#include <ncore/core/rid.h>
#include <ncore/core/vector.h>
#include <ncore/resources/cube_map.h>
#include <ncore/services/service.h>

#include "renderer/render_context.h"
#include "renderer/render_storage.h"
#include "renderer/vertex_format.h"
#include "rhi.h"

namespace nc {

class Image;
class Shader;
class Mesh;

/**
 * @brief RenderService keeps all funtionalities for rendering stuff on-screen and off-screen.
 */
class NCAPI RenderService : public IService {
    NCLASS( RenderService, IService )

public:
    struct NCAPI RenderSettings {
        bool VSync = true;
        NSTRUCTV( RenderSettings, NC_F( RenderSettings, VSync ) )
    };

    const RenderSettings& get_settings() const
    {
        return settings;
    }

    void set_settings( const RenderSettings& p_settings )
    {
        settings = p_settings;
    }

    bool initialize() override;
    void shutdown() override;
    void update( float dt ) override;

    IRHI* get_rhi() const { return rhi.get(); }

    // ---- textures ----
    RID texture_2d_create( const Image& image );
    RID texture_2d_create( const rhi::TextureDesc& desc );
    RID texture_cube_create( const CubeMap& cubemap );

    void texture_data_update( RID texture, const void* data, size_t size );

    // ---- buffers ----
    RID buffer_create( const rhi::BufferDesc& desc );
    void buffer_data_write( RID buffer, const void* data, size_t size, size_t offset = 0 );
    void buffer_blit( RID p_src_buffer, RID p_dst_buffer );

    RID resource_set_create( const Shader& shader, uint8_t set_idx, Span<const rhi::ResourceMappingEntry> p_resources );
    void resource_set_bind( RID p_resource_set );

    /**
     * @brief Create a GPU material from a Shader (no MaterialTemplate).
     */
    RID material_create( const Shader& shader, const MaterialCreateDesc& desc = {} );
    void material_set_texture( RID material, RID texture, uint32_t slot );
    void material_set_draw_mode( RID material, rhi::FillMode mode );

    RID gpu_mesh_create( const Mesh& mesh );

    RID compute_pipeline_create( const Shader& shader, Span<const RID> p_resource_sets );
    void compute_pipeline_bind( RID pipeline );
    void compute_dispatch( uint32_t x, uint32_t y, uint32_t z );

    void spatial_draw_instance( RID mesh, const Mat4& transform, RID material, uint32_t instance_count = 1 );
    void canvas_draw_rect( const Rect2& rect, const Color& color, RID texture, RID material );

    bool is_rid_owned( RID rid );
    bool destroy_rid( RID rid );
    void flush_pending_destroys();

    RenderContext& get_context() { return context; }
    RenderStorage& get_storage() { return storage; }

private:
    RenderSettings settings;
    std::unique_ptr<IRHI> rhi;
    RenderContext context;
    RenderStorage storage;
};

} // namespace nc
