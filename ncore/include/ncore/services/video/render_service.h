#pragma once

#include <memory>

#include <ncore/core/collection.h>
#include <ncore/core/color.h>
#include <ncore/core/matrix.h>
#include <ncore/core/rid.h>
#include <ncore/core/vector.h>
#include <ncore/services/service.h>

#include "renderer/renderer_context.h"
#include "renderer/renderer_storage.h"
#include "renderer/vertex_format.h"
#include "rhi.h"

namespace nc {

class Image;
class Shader;
class MaterialTemplate;
class Mesh;

/**
 * @brief RenderService keeps all funtionalities for rendering stuff on-screen,
 * and perhaps off-screen stuff too later.
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

    Error init( ConfFile& cfg_file ) override;
    void shutdown() override;

    RID swapchain_create( void* whnd, Vec2 size );
    void swapchain_set_size( RID sc, Vec2 size );
    Vec2 swapchain_get_size( RID sc );
    void swapchain_destroy( RID sc );

    RID texture_2d_create(
        uint32_t width, uint32_t height, TextureFormat format = TextureFormat::RGBA8_UNORM,
        ResourceBindFlags bind_mask = ResourceBindFlags::SHADER_RESOURCE
    );
    RID texture_2d_create( const Image& image );

    RID texture_cube_create( Span<const Image*, 6> faces );

    RID material_create( const MaterialTemplate& tmpl );
    void material_set_texture( RID material, RID texture, uint32_t slot );

    /**
     * @brief Uploads a CPU Mesh resource into the current GPU device.
     * @return A new RID handle to the buffer.
     */
    RID gpu_mesh_create( const Mesh& mesh );

    /**
     * @brief Destroy any previously allocated RIDs from methods
     * in this class that return RID.
     */
    void destroy_rid( RID rid );

    RID compute_pipeline_create( const ComputePSODesc& desc );
    void compute_pipeline_bind( RID pipeline );
    void dispatch( uint32_t x, uint32_t y, uint32_t z );

    void compute_texture_bind( RID texture, RID binding, const char* name, TextureViewType view );
    void compute_buffer_bind( RID buffer, RID binding, const char* name );

    RID resource_signature_create( const ResourceSignatureDesc& desc );
    RID resource_binding_create( RID signature );
    void resource_binding_commit( RID binding );

    RID buffer_create( const BufferDesc& desc );
    void buffer_update( RID buffer, const void* data, size_t size );

    /**
     * @brief Begin a new frame.
     * Clears previous one.
     */
    void frame_begin();
    /**
     * @brief Flush current frame.
     */
    void frame_end( float delta_time );

    float world_camera_get_fov() const;
    void world_camera_set_fov( float fov );
    float world_camera_get_z_near() const;
    void world_camera_set_z_near( float p_near );
    float world_camera_get_z_far() const;
    void world_camera_set_z_far( float p_far );
    Mat4 world_camera_get_transform() const;
    void world_camera_set_transform( const Mat4& transform );
    Mat4 world_camera_get_projection() const;

    Mat4 world_get_view_matrix() const;

    /**
     * @brief Draw meshes.
     *
     * Pushes a new 3D draw call to the draw list to be rendered next frame.
     */
    void world_draw_instance( RID gpu_mesh, const Mat4& transform, RID material, uint32_t instancing = 1 );

    /**
     * @brief Immediate draw an array of indexed vertices.
     *
     * Pushes a new Canvas Item draw call to the draw list to be rendered next frame.
     */
    void canvas_draw_triangles(
        std::span<const Vertex2D> verts, std::span<const uint16_t> indices, RID material, Rect clip = {}
    );

    /**
     * @brief Immediate draw a simple 2D rectangle.
     *
     * Pushes a new Canvas Item draw call to the draw list to be rendered next frame.
     */
    void canvas_draw_quad(
        Vec2 points[4], RID material, Color tint = Color( 255, 255, 255, 255 ),
        Rect uv_rect = Rect( 0.0f, 0.0f, 1.0f, 1.0f ), Rect clip = {}
    );

    /**
     * @brief Return the internal RHI and storage for advanced use.
     */
    RendererContext* get_context()
    {
        return &ctx;
    }

    /**
     * @brief Query statistics.
     */
    IRHI::Stats get_stats() const;

private:
    void ensure_canvas_vb_( uint32_t needed );
    void ensure_canvas_ib_( uint32_t needed );

    RenderSettings settings;
    RendererContext ctx;
    // WorldRenderer m_world;
    DynamicArray<RID> swapchains;
    float time;

    struct Camera {
        Mat4 transform  = Mat4::identity();
        Mat4 projection = Mat4::identity();
        float fov       = 1.5708f; // a.k.a angle-of-view (in radians).
        float z_near    = 0.1f;    // Near clipping plane.
        float z_far     = 100.0f;  // Far clipping plane.
    };

    Camera main_cam;
    Mat4 view_matrix = Mat4::identity();

    // Canvas
    Mat4 ortho_proj = Mat4::identity();
    RID canvas_vb;
    RID canvas_ib;
    uint32_t canvas_vb_size = 0;
    uint32_t canvas_ib_size = 0;
};

} // namespace nc
