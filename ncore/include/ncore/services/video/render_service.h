#pragma once

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
class MaterialTemplate;
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

    Error init( ConfFile& cfg_file ) override;
    void shutdown() override;

    RID swapchain_create(
        void* whnd, Vec2i size, TextureFormat color_format = TextureFormat::RGBA8_UNORM_SRGB,
        TextureFormat depth_format = TextureFormat::D32_FLOAT
    );
    /**
     * @brief Set the width and height of given swapchain.
     */
    void swapchain_set_size( RID swapchain, Vec2i size );
    /**
     * @brief Return the width and height of given swapchain.
     */
    Vec2i swapchain_get_size( RID swapchain );
    /**
     * @brief Return the primary swapchain RID.
     */
    RID swapchain_get_primary() const;
    void swapchain_destroy( RID swapchain );

    RID texture_2d_create( const Image& image );
    /**
     * @brief Create a cube mapped texture from 6 separate images (faces).
     */
    RID texture_cube_create( const CubeMap& cubemap );
    /**
     * @brief Create a new render texture.
     * @return RenderTexture RID.
     */
    RID texture_render_create( Vec2i size, TextureFormat format = TextureFormat::RGBA8_UNORM );
    /**
     * @brief Retrieve a texture view.
     */
    void* texture_get_view( RID texture, TextureViewType view );
    /**
     * @brief Copy data from source texture into destination texture.
     * @param tex_src Source texture to copy from.
     * @param tex_dest Destination texture. If none (0), target is the primary swapchain.
     */
    void texture_blit( RID tex_src, RID tex_dest = 0 );

    /**
     * @brief Instantiates a material from its template.
     * A material has its own textures.
     */
    RID material_create( const MaterialTemplate& tmpl );
    void material_set_texture( RID material, RID texture, uint32_t slot );
    void material_set_draw_mode( RID material, FillMode mode );

    /**
     * @brief Uploads a CPU Mesh resource into the current GPU device.
     * @return A new RID handle to the buffer.
     */
    RID gpu_mesh_create( const Mesh& mesh );

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

    struct CameraAttribs {
        Mat4 Transform = Mat4::identity();
        float Fov      = 1.5708f; // a.k.a angle-of-view (in radians).
        float zNear    = 0.1f;    // Near clipping plane.
        float zFar     = 100.0f;  // Far clipping plane.
        Vec2i DisplaySize;
    };

    RID camera_create();
    CameraAttribs& camera_get_attribs( RID camera );
    Mat4 camera_get_perspective( RID camera );

    bool is_rid_owned( RID rid );
    /**
     * @brief Destroy any previously allocated RIDs from methods
     * in this class that return RID.
     * @return True if succesfully destroyed.
     */
    bool destroy_rid( RID rid );

    /**
     * @brief Begin a new frame. Must be called once before render_pass().
     */
    void render_begin( float delta_time );

    /**
     * @brief Describes a single render pass target and camera.
     */
    struct RenderPassDesc {
        RID color_target;   // RenderTexture RID.
        RID depth_target;   // RenderTexture RID.
        Rect2i target_rect; // Target dimensions.
        RID camera;         // A spatial camera. Ignored during canvas draw.
        Color clear_color = Color( 0, 0, 0, 255 );
        bool clear        = true;
        bool draw_canvas  = true; // Skips 2D render if false.
        bool draw_spatial = true; // Skips 3D render if false.
        bool to_screen    = false;
    };

    /**
     * @brief Execute one render pass into the described target.
     */
    void render_pass( const RenderPassDesc& desc );

    /**
     * @brief Present and end the frame. Must be called after all render_pass() calls.
     */
    void present();

    /**
     * @brief Draw spatial (3D) meshes.
     *
     * Pushes a new 3D draw call to the draw list to be rendered next frame.
     */
    void spatial_draw_instance( RID gpu_mesh, const Mat4& transform, RID material, uint32_t instancing = 1 );

    /**
     * @brief Immediate draw an array of indexed vertices.
     *
     * Pushes a new Canvas draw call to the draw list to be rendered next frame.
     */
    void canvas_draw_triangles(
        std::span<const Vertex2D> verts, std::span<const uint16_t> indices, RID material, RID texture = 0,
        Rect2i clip = {}
    );

    /**
     * @brief Immediate draw a simple 2D rectangle.
     *
     * Pushes a new Canvas Item draw call to the draw list to be rendered next frame.
     */
    void canvas_draw_quad(
        Vec2f points[4], RID material, RID texture = 0, Color tint = Color( 255, 255, 255, 255 ),
        Rect2i uv_rect = Rect2i( 0, 0, 1, 1 ), Rect2i clip = {}
    );

    /**
     * @brief Return the internal render hardware interface for advanced use.
     */
    IRHI* get_graphics_api()
    {
        return gfx_api.get();
    }

    /**
     * @brief Return the internal render context for advanced use.
     */
    RenderContext* get_context()
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
    Ptr<IRHI> gfx_api;
    RenderContext ctx;
    RenderStorage storage; // Access to high-level GPU-bound resources.
    DynamicArray<RID> swapchains;
    RIDPool<CameraAttribs> cameras{ 16 };
    float time;
    float last_dt_ = 0.0f;

    // Canvas
    RID canvas_vb;
    RID canvas_ib;
    uint32_t canvas_vb_size = 0;
    uint32_t canvas_ib_size = 0;
};

} // namespace nc
