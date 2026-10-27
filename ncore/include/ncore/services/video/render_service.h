#pragma once

#include <memory>

#include <ncore/core/collection.h>
#include <ncore/core/color.h>
#include <ncore/core/matrix.h>
#include <ncore/core/rid.h>
#include <ncore/core/vector.h>
#include <ncore/resources/resource.h>
#include <ncore/services/service.h>

#include "gfx_interface.h"
#include "renderer/render_context.h"
#include "renderer/render_storage.h"
#include "renderer/vertex_format.h"

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
    struct REFLECT NCAPI RenderSettings {
        REFLECT bool vsync = true;
    };

    const RenderSettings& get_settings() const [[clang::lifetimebound]]
    {
        return settings;
    }

    Error init( ConfFile& cfg_file ) override;
    void shutdown() override;

    RID swapchain_create(
        void* whnd, Vec2i size, gfx::TextureFormat color_format = gfx::TextureFormat::RGBA8_UNORM_SRGB,
        gfx::TextureFormat depth_format = gfx::TextureFormat::D32_FLOAT
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
     * @brief Return the primary swapchain RID. Invalid RID if none exists.
     */
    RID swapchain_get_primary() const;
    void swapchain_destroy( RID swapchain );

    RID texture_2d_create( const Image& image, bool updatable = false );
    void texture_data_update(
        RID texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t row_pitch_bytes,
        const void* p_data
    );
    /**
     * @brief Create a cube mapped texture from 6 separate images (faces).
     */
    RID texture_cube_create( const CubeMap& cubemap );
    /**
     * @brief Create a new renderable texture, sampleable by shaders.
     * @return render_texture RID.
     */
    RID texture_render_create( Vec2i size, gfx::TextureFormat format = gfx::TextureFormat::RGBA8_UNORM );
    /**
     * @brief Retrieve a texture view.
     */
    void* texture_view_get( RID texture, gfx::TextureViewType view );
    /**
     * @brief Copy data from source texture into destination texture.
     * @param p_src Source texture to copy from.
     * @param p_dest Destination texture. If none (0), target is the primary swapchain.
     */
    void texture_blit( RID p_src, RID p_dest = 0 );

    RID buffer_create( const gfx::BufferDesc& desc );
    /**
     * @brief Change what's inside an existing buffer.
     * @param p_buffer The existing GPU buffer.
     * @param p_src Source data to copy into the buffer.
     */
    void buffer_data_write( RID p_buffer, const void* p_src, size_t p_src_size );
    /**
     * @brief Read data from an existing buffer.
     */
    void buffer_data_read( RID p_buffer, void* p_dst, size_t p_dst_size );
    /**
     * @brief Copy data from one buffer to another.
     * @param p_src_buffer RID handle of source buffer.
     * @param p_dst_buffer RID handle of destination buffer.
     */
    void buffer_blit( RID p_src_buffer, RID p_dst_buffer );

    RID resource_set_create( const Shader& shader, uint8_t set_idx, Span<const gfx::ResourceMappingEntry> p_resources );
    /**
     * @brief Commit resources in the set to the device context.
     */
    void resource_set_bind( RID p_resource_set );

    /**
     * @brief Create GPU-side material instance from Material resource.
     *
     * A material has its own shader resources.
     */
    RID material_create( const Ref<MaterialShader>& p_shader );
    void material_set_param( RID p_material, const String& p_name, const void* p_data, size_t p_data_size );
    void material_set_texture( RID material, RID texture, uint32_t slot );
    void material_set_draw_mode( RID material, gfx::FillMode mode );

    /**
     * @brief Uploads a CPU Mesh resource into the current GPU device.
     * @return A new RID handle to the mesh buffer object.
     */
    RID mesh_create( const Mesh& mesh );

    RID compute_pipeline_create( const Shader& shader, Span<const RID> p_resource_sets );
    void compute_pipeline_bind( RID pipeline );
    /**
     * @brief Executes a dispatch compute command.
     * @param x The number of thread groups dispatch in X direction.
     * @param y The number of thread groups dispatch in Y direction.
     * @param z The number of thread groups dispatch in Z direction.
     */
    void compute_dispatch( uint32_t x, uint32_t y, uint32_t z );

    struct CameraAttribs {
        Mat4 transform = Mat4::identity();
        float fov      = 1.5708f; // a.k.a angle-of-view (in radians).
        float z_near    = 0.1f;    // Near clipping plane.
        float z_far     = 100.0f;  // Far clipping plane.
        Vec2i display_size;

        // cached perspective projection.
        // recomputed only when the projection inputs change.
        Mat4 cached_proj;
        float cached_fov     = -1.0f;
        float cached_znear   = 0.0f;
        float cached_zfar    = 0.0f;
        Vec2i cached_display = {};
    };

    RID camera_create();
    CameraAttribs* camera_attribs_get( RID camera );
    Mat4 camera_perspective_get( RID camera );

    RID spatial_item_create();
    void spatial_item_set_mesh( RID p_spatial_item, RID p_gpu_mesh );
    /**
     * @brief Get GPU mesh bound to a spatial item (0 when none/unowned).
     */
    RID spatial_item_get_mesh( RID p_spatial_item );
    void spatial_item_set_transform( RID p_spatial_item, const Mat4& p_transform );
    void spatial_item_set_material( RID p_spatial_item, RID p_material );
    /**
     * @brief Draw a spatial (3D) item.
     *
     * Pushes a new 3D draw command to the draw list to be rendered next frame.
     */
    void spatial_item_draw( RID p_spatial_item, uint16_t p_layer_index = 0, uint32_t p_instancing = 1 );

    RID canvas_item_create();
    void canvas_item_set_material( RID p_canvas_item, RID p_material );
    void canvas_item_set_clipping( RID p_canvas_item, Rect2i p_clip );
    void canvas_item_add_triangles( RID p_canvas_item, Span<const Vertex2D> p_verts, Span<const uint32_t> p_indices );
    /**
     * @brief Add a simple 2D rectangle to the canvas item.
     */
    void canvas_item_add_quad(
        RID p_canvas_item, Vec2f p_points[4], Rect2i p_uv_rect = Rect2i( 0, 0, 1, 1 ),
        Color p_tint = Color( 255, 255, 255, 255 ), RID p_texture = 0
    );
    /**
     * @brief Draw a canvas (2D) item.
     *
     * Pushes a new Canvas draw call to the draw list to be rendered next frame.
     */
    void canvas_item_draw( RID p_canvas_item, uint32_t z_order = 0 );
    void canvas_item_draw(
        RID p_canvas_item, Span<const Vertex2D> p_verts_override, Span<const uint32_t> p_indices_override,
        RID p_texture_override, Rect2i p_clip_override, uint32_t z_order = 0
    );

    /**
     * @brief Prepare a new frame. Must be called once before render_target().
     */
    void prepare_frame( float delta_time );

    /**
     * @brief Describes a single render target and camera.
     */
    struct RenderTargetDesc {
        RID color_texture; // Target color render_texture RID.
        RID depth_texture; // Target depth render_texture RID.
        Rect2i rectangle;  // Target render dimensions.
        Color clear_color = Color( 0, 0, 0, 255 );
        bool clear        = true;
        bool draw_canvas  = true; // Skips 2D render if false.
        bool draw_spatial = true; // Skips 3D render if false.
        bool to_screen    = false;
        RID camera;               // A spatial camera. Ignored during canvas draw.
    };

    /**
     * @brief Execute one render operation into the described target.
     */
    void render_target( const RenderTargetDesc& p_desc );

    /**
     * @brief Present and end the frame. Must be called after all render_target() calls.
     */
    void present();

    bool is_rid_owned( RID rid );
    /**
     * @brief Destroy any previously allocated RIDs from methods
     * in this class that return RID.
     * @return True if succesfully destroyed.
     */
    bool destroy_rid( RID rid );

    /**
     * @brief Return the internal render hardware interface for advanced use.
     */
    GfxInterface* get_graphics_api() [[clang::lifetimebound]]
    {
        return gfx_api.get();
    }

    /**
     * @brief Return the internal render context for advanced use.
     */
    RenderContext* get_context() [[clang::lifetimebound]]
    {
        return &ctx;
    }

    struct RenderStats {
        static constexpr int kMaxPassTimestamps = 8;

        // Per-frame render-pass durations (one entry per render_target call) in ms.
        // Only pass_count entries are valid.
        uint32_t pass_count                         = 0;
        double pass_duration_ms[kMaxPassTimestamps] = {};
    };

    /**
     * @brief Return the render statistics result for one frame.
     */
    RenderStats get_stats() const
    {
        return m_stats;
    }

private:
    void ensure_canvas_vertex_buf_( uint32_t p_needed_verts );
    void ensure_canvas_index_buf_( uint32_t p_needed_ids );
    Mat4 camera_perspective_get_impl( CameraAttribs& attribs );

    RenderSettings settings;
    Ptr<GfxInterface> gfx_api;
    RID gfx_device_ctx;    // The handle of current RHI device context for gfx ops
    RenderContext ctx;
    RenderStorage storage; // Access to high-level GPU-bound resources.
    HashSet<RID> swapchains;
    RID primary_swapchain;
    RIDPool<CameraAttribs> cameras{ 16 };
    float time;
    float last_dt = 0.0f;

    // Per-frame GPU timing of each render_target() pass (see get_stats()).
    RenderStats m_stats;

    // Canvas
    RID canvas_vbo;
    RID canvas_ibo;
    uint32_t canvas_vbo_size = 0;
    uint32_t canvas_ibo_size = 0;

    struct SpatialInstanceData {
        Mat4 model_matrix;
        Mat4 normal_matrix;
    };

    // Keeps the C++ block, the size Diligent publishes to VkPushConstantRange, and
    // ConstantBuffer<SpatialInstance> in spatial.slang in lockstep.
    static_assert(
        sizeof( SpatialInstanceData ) == RenderStorage::SPATIAL_PUSH_CONSTANT_UINTS * 4,
        "SpatialInstanceData must match spatial.slang SpatialInstance and SPATIAL_PUSH_CONSTANT_UINTS"
    );

    struct CanvasInstanceData {
        Mat4 view_proj_matrix; // 2D ortho projection for the canvas pass
    };

    static_assert(
        sizeof( CanvasInstanceData ) == RenderStorage::CANVAS_PUSH_CONSTANT_UINTS * 4,
        "CanvasInstanceData must match canvas.slang CanvasInstance and CANVAS_PUSH_CONSTANT_UINTS"
    );
};

} // namespace nc
