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

    const RenderSettings& get_settings() const { return settings; }
    void set_settings( const RenderSettings& p_settings ) { settings = p_settings; }

    bool initialize() override;
    void shutdown() override;

    RID swapchain_create(
        void* window_handle, Vec2i size, rhi::TextureFormat format = rhi::TextureFormat::RGBA8_UNORM_SRGB,
        bool vsync = true
    );
    void swapchain_set_size( RID swapchain, Vec2i size );
    Vec2i swapchain_get_size( RID swapchain );
    RID swapchain_get_primary() const;
    void swapchain_destroy( RID swapchain );

    RID texture_2d_create( const Image& image );
    RID texture_cube_create( const CubeMap& cubemap );
    RID texture_render_create( Vec2i size, rhi::TextureFormat format = rhi::TextureFormat::RGBA8_UNORM );
    void* texture_view_get( RID texture, rhi::TextureViewType view );
    void texture_blit( RID tex_src, RID tex_dest = 0 );

    RID buffer_create( const rhi::BufferDesc& desc );
    void buffer_data_write( RID p_buffer, Span<const std::byte> p_src );
    void buffer_data_read( RID p_buffer, Span<std::byte> p_dst );
    void buffer_blit( RID p_src_buffer, RID p_dst_buffer );

    RID resource_set_create( const Shader& shader, uint8_t set_idx, Span<const rhi::ResourceMappingEntry> p_resources );
    void resource_set_bind( RID p_resource_set );

    /**
     * @brief Create a GPU material from a Shader (MaterialTemplate removed).
     */
    RID material_create( const Shader& shader, const MaterialCreateDesc& desc = {} );
    void material_set_texture( RID material, RID texture, uint32_t slot );
    void material_set_draw_mode( RID material, rhi::FillMode mode );

    RID gpu_mesh_create( const Mesh& mesh );

    RID compute_pipeline_create( const Shader& shader, Span<const RID> p_resource_sets );
    void compute_pipeline_bind( RID pipeline );
    void compute_dispatch( uint32_t x, uint32_t y, uint32_t z );

    RID camera_create();
    struct CameraAttribs;
    CameraAttribs& camera_get_attribs( RID camera );
    Mat4 camera_get_perspective( RID camera );

    bool is_rid_owned( RID rid );
    bool destroy_rid( RID rid );

    void render_begin( float delta_time );

    struct RenderPassDesc {
        RID color_target;
        RID depth_target;
        RID camera;
        Color clear_color = Color( 0, 0, 0, 255 );
        float clear_depth = 1.0f;
        bool clear = true;
    };
    void render_pass( const RenderPassDesc& desc );
    void present();

    void spatial_draw_instance( RID gpu_mesh, const Mat4& transform, RID material, uint32_t instancing = 1 );
    void canvas_draw_triangles(
        Span<const Vertex2D> verts, Span<const uint16_t> indices, RID material, RID texture = 0, Rect2i clip = {}
    );
    void canvas_draw_quad(
        Vec2f points[4], RID material, RID texture = 0, Color tint = Color( 255, 255, 255, 255 ), Rect2i clip = {}
    );

    IRHI* get_rhi() { return rhi.get(); }
    RenderStorage& get_storage() { return storage; }
    RenderContext& get_context() { return context; }

private:
    void ensure_canvas_vb_( uint32_t needed );
    void ensure_canvas_ib_( uint32_t needed );

    RenderSettings settings;
    std::unique_ptr<IRHI> rhi;
    RenderContext context;
    RenderStorage storage;

    RID canvas_vb;
    RID canvas_ib;
};

} // namespace nc
