#if defined( _WIN32 )
#define NOMINMAX
#endif

#include <cmath>

#include <backends/diligent/rhi_diligent.h>

#include <ncore/core/matrix.h>
#include <ncore/core/rect.h>
#include <ncore/core/vector.h>
#include <ncore/resources/image.h>
#include <ncore/resources/material_template.h>
#include <ncore/resources/mesh.h>
#include <ncore/services/video/render_service.h>
#include <ncore/services/video/renderer/vertex_format.h>
#include <ncore/utils/config.h>
#include <ncore/utils/log.h>

namespace nc {

Error RenderService::init( ConfFile& cfg_file )
{
    settings = cfg_file.read<RenderSettings>();

    ctx.rhi = new DiligentRHI();
    ctx.rhi->load_pso_cache();
    ctx.storage.set_rhi( ctx.rhi );

    ctx.gfx_device_ctx = ctx.rhi->create_deferred_context( IRHI::GpuQueue::Graphics );

    return Error::OK;
}

void RenderService::shutdown()
{
    for (auto& sc : swapchains) {
        ctx.rhi->swapchain_destroy( sc );
    }
    swapchains.clear();

    if (canvas_vb.is_valid())
        ctx.rhi->destroy_resource( canvas_vb );
    if (canvas_ib.is_valid())
        ctx.rhi->destroy_resource( canvas_ib );

    ctx.storage.flush_pending_destroys();
    ctx.rhi->save_pso_cache();
    delete ctx.rhi;
}

RID RenderService::swapchain_create( void* whnd, Vec2 size )
{
    RID rid =
        ctx.rhi->swapchain_create( SwapChainDesc{ .native_whnd = whnd, .initial_size = size, .is_primary = true } );
    swapchains.push_back( rid );
    return rid;
}

void RenderService::swapchain_set_size( RID sc, Vec2 size )
{
    ctx.rhi->swapchain_set_size( sc, size );
}

Vec2 RenderService::swapchain_get_size( RID sc )
{
    return ctx.rhi->swapchain_get_size( sc );
}

void RenderService::swapchain_destroy( RID sc )
{
    ctx.rhi->swapchain_destroy( sc );
    std::erase( swapchains, sc );
}

RID RenderService::texture_2d_create(
    uint32_t width, uint32_t height, TextureFormat format, ResourceBindFlags bind_mask
)
{
    return ctx.rhi->texture_create(
        TextureDesc{
            .debug_name = "RenderService_FloatTexture",
            .format     = format,
            .dimension  = ResourceDimension::DIM_2D,
            .usage      = ResourceUsage::DEFAULT,
            .bind_mask  = bind_mask,
            .width      = width,
            .height     = height,
            .faces      = {},
        }
    );
}

RID RenderService::texture_2d_create( const Image& image )
{
    return ctx.rhi->texture_create(
        TextureDesc{
            .debug_name  = image.filepath,
            .format      = TextureFormat::RGBA8_UNORM_SRGB,
            .dimension   = ResourceDimension::DIM_2D,
            .usage       = ResourceUsage::DYNAMIC,
            .access_mask = ResourceAccessFlags::WRITE,
            .width       = image.get_width(),
            .height      = image.get_height(),
            .pixels      = image.get_pixels().data(),
            .faces       = {},
        }
    );
}

RID RenderService::texture_cube_create( const CubeMap& cube_map )
{
    auto faces = cube_map.get_faces();

    TextureDesc desc;
    auto name       = std::format( "CubeTexture_{}_{}", cube_map.rid.value, cube_map.filepath );
    desc.debug_name = name;
    desc.format     = TextureFormat::RGBA8_UNORM_SRGB;
    desc.dimension  = ResourceDimension::DIM_CUBE;
    desc.usage      = ResourceUsage::IMMUTABLE;
    desc.width      = faces[0]->get_width();
    desc.height     = faces[0]->get_height();
    desc.array_size = 6;
    for (int i = 0; i < 6; i++) {
        desc.faces[i] = faces[i]->get_pixels().data();
    }
    return ctx.rhi->texture_create( desc );
}

RID RenderService::material_create( const MaterialTemplate& tmpl )
{
    return ctx.storage.material_create( tmpl );
}

void RenderService::material_set_texture( RID material, RID texture, uint32_t slot )
{
    ctx.storage.material_set_texture( material, texture, slot );
}

void RenderService::material_set_draw_mode( RID material, FillMode mode )
{
    ctx.storage.material_set_draw_mode( material, mode );
}

RID RenderService::gpu_mesh_create( const Mesh& mesh )
{
    return ctx.storage.gpu_mesh_create( mesh );
}

void RenderService::destroy_rid( RID rid )
{
    ctx.storage.destroy_rid( rid );
}

RID RenderService::compute_pipeline_create( const ComputePSODesc& desc )
{
    return ctx.rhi->compute_pipeline_create( desc );
}

void RenderService::compute_pipeline_bind( RID pipeline )
{
    ctx.rhi->set_queue( IRHI::GpuQueue::Compute );
    ctx.rhi->compute_pipeline_bind( pipeline );
}

void RenderService::dispatch( uint32_t x, uint32_t y, uint32_t z )
{
    ctx.rhi->set_queue( IRHI::GpuQueue::Compute );
    ctx.rhi->dispatch( x, y, z );
}

void RenderService::compute_texture_bind( RID texture, RID binding, const char* name, TextureViewType view )
{
    ctx.rhi->texture_compute_update( texture, binding, name, view );
}

void RenderService::compute_buffer_bind( RID buffer, RID binding, const char* name )
{
    ctx.rhi->buffer_compute_update( buffer, binding, name );
}

RID RenderService::resource_signature_create( const ResourceSignatureDesc& desc )
{
    return ctx.rhi->resource_signature_create( desc );
}

RID RenderService::resource_binding_create( RID signature )
{
    return ctx.rhi->resource_binding_create( signature );
}

void RenderService::resource_binding_commit( RID binding )
{
    ctx.rhi->resource_binding_commit( binding );
}

RID RenderService::buffer_create( const BufferDesc& desc )
{
    return ctx.rhi->buffer_create( desc );
}

void RenderService::buffer_update( RID buffer, const void* data, size_t size )
{
    ctx.rhi->buffer_update( buffer, data, size );
}

void RenderService::frame_begin()
{
    ctx.rhi->set_queue( IRHI::GpuQueue::Graphics );
    ctx.rhi->set_context_state( false );
    // ctx.rhi->set_context_state( true, ctx.rhi_ctx );
    // ctx.rhi->commands_record_begin();

    // TODO: handle multiple swapchains
    auto rtv = ctx.rhi->swapchain_get_view( swapchains[0], TextureViewType::RENDER_TARGET );
    auto dsv = ctx.rhi->swapchain_get_view( swapchains[0], TextureViewType::DEPTH_STENCIL );

    NC_LOG_TRACE_C(
        log::GRAPHICS, "frame_begin: rtv={} dsv={}", reinterpret_cast<uintptr_t>( rtv ),
        reinterpret_cast<uintptr_t>( dsv )
    );

    auto size = ctx.rhi->swapchain_get_size( swapchains[0] );

    // clang-format off
    // set ortho proj matrix
	ortho_proj = Mat4(
	    Vec4( 2.0f / size.x, 0.0f,           0.0f,  0.0f ),
	    Vec4( 0.0f,          -2.0f / size.y, 0.0f,  0.0f ),
	    Vec4( 0.0f,          0.0f,           1.0f,  0.0f ),
	    Vec4( -1.0f,         1.0f,           0.0f,  1.0f )
	);
    // clang-format on
    // set perspective proj matrix
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix//building-basic-perspective-projection-matrix.html
    // https://github.com/DiligentGraphics/DiligentSamples/blob/master/SampleBase/src/SampleBase.cpp
    auto aspect_ratio = size.x / size.y;
    float y_scale     = 1.0f / std::tan( main_cam.fov * 0.5f );
    float x_scale     = y_scale / aspect_ratio;
    auto n            = main_cam.z_near;
    auto f            = main_cam.z_far;
    auto zz           = -f / ( f - n );     // used to remap z to [0,1]
    auto wz           = -f * n / ( f - n ); // used to remap z [0,1]
    // clang-format off
    main_cam.projection = Mat4(
        Vec4( x_scale, 0,       0,   0  ), // scale the x coordinates of the projected point
        Vec4( 0,       y_scale, 0,   0  ), // scale the y coordinates of the projected point
        Vec4( 0,       0,       zz,  -1 ), // set w = -z
		Vec4( 0,       0,       wz,  0  )  
    );
    // clang-format on

    Rect full_scissor( 0, 0, size.x, size.y );
    ctx.rhi->render_target_set_scissor_rect( { &full_scissor, 1 } );

    const void* rtvs[] = { rtv };
    ctx.rhi->render_target_bind( rtvs, dsv );
    ctx.rhi->render_target_clear_color( rtv, Color( 0, 0, 0, 255 ) ); // TODO: don't hardcode grey
    ctx.rhi->render_target_clear_depth( dsv );

    IRHI::Viewport vp{ .rect = Rect( 0, 0, size.x, size.y ) };
    ctx.rhi->render_target_set_viewport( { &vp, 1 } );
}

void RenderService::frame_end( float delta_time )
{
    NC_LOG_TRACE_C( log::GRAPHICS, "frame_end: canvas_render_list={}", ctx.canvas_render_list.size() );

    time += delta_time;

    ctx.rhi->begin_queries();

    RendererStorage::ShaderConstants constants;
    // precompute the V*P part of M*V*P so we don't have do it on the GPU.
    // here we take the inverse of camera transform to get its view matrix.
    view_matrix              = main_cam.transform.affine_inverse();
    constants.CameraMatrix   = main_cam.transform;
    constants.ViewProjMatrix = main_cam.projection * view_matrix;
    constants.Time           = time;
    constants.DeltaTime      = delta_time;

    for (auto& item : ctx.world_render_list) {
        NC_ASSERT( item.material.is_valid(), "A valid material is required to draw a world object." );
        NC_LOG_TRACE_C(
            log::GRAPHICS, "world_items_flush: gpu_mesh_rid={} instances={}", item.gpu_mesh.value, item.instancing
        );
        auto mesh                = ctx.storage.get_gpu_mesh( item.gpu_mesh );
        constants.ModelMatrix    = item.transform;
        constants.ModelMatrixInv = item.transform.affine_inverse();
        ctx.storage.material_bind( item.material, constants );
        ctx.storage.gpu_mesh_bind( item.gpu_mesh );
        ctx.rhi->draw_indexed( mesh->index_count, 0, 0, item.instancing );
    }

    constants.ModelMatrix    = Mat4::identity();
    constants.ViewProjMatrix = ortho_proj;

    for (auto& item : ctx.canvas_render_list) {
        if (item.verts.empty()) {
            NC_LOG_TRACE_C( log::GRAPHICS, "canvas_items_flush: skipped (empty)" );
            continue;
        }

        NC_LOG_TRACE_C(
            log::GRAPHICS, "canvas_items_flush: verts={} indices={} material_rid={}", item.verts.size(),
            item.indices.size(), item.material.value
        );

        ensure_canvas_vb_( static_cast<uint32_t>( item.verts.size() ) );
        ensure_canvas_ib_( static_cast<uint32_t>( item.indices.size() ) );

        ctx.rhi->buffer_update( canvas_vb, item.verts.data(), item.verts.size() * sizeof( Vertex2D ) );
        ctx.rhi->buffer_update( canvas_ib, item.indices.data(), item.indices.size() * sizeof( uint16_t ) );

        ctx.storage.material_bind( item.material, constants );

        ctx.rhi->vertex_buffers_bind( { &canvas_vb, 1 }, 0 );
        ctx.rhi->index_buffer_bind( canvas_ib, 0 );

        if (item.clip.x >= 0 && item.clip.y >= 0 && item.clip.w > 0 && item.clip.h > 0) {
            ctx.rhi->render_target_set_scissor_rect( { &item.clip, 1 } );
        }

        NC_LOG_TRACE_C( log::GRAPHICS, "canvas_items_flush: draw indexed (indices={})", item.indices.size() );
        ctx.rhi->draw_indexed( static_cast<uint32_t>( item.indices.size() ), 0, 0 );
    }

    ctx.rhi->end_queries();

    ctx.rhi->swapchain_present( swapchains[0], settings.VSync );

    // ctx.rhi->commands_release();

    ctx.storage.flush_pending_destroys();

    ctx.world_render_list.reset();
    ctx.canvas_render_list.release_all(); // CanvasRenderItem is non-POD
}

float RenderService::world_camera_get_fov() const
{
    return main_cam.fov;
}

void RenderService::world_camera_set_fov( float fov )
{
    main_cam.fov = fov;
}

float RenderService::world_camera_get_z_near() const
{
    return main_cam.z_near;
}

void RenderService::world_camera_set_z_near( float p_near )
{
    main_cam.z_near = p_near;
}

float RenderService::world_camera_get_z_far() const
{
    return main_cam.z_far;
}

void RenderService::world_camera_set_z_far( float p_far )
{
    main_cam.z_far = p_far;
}

Mat4 RenderService::world_camera_get_transform() const
{
    return main_cam.transform;
}

void RenderService::world_camera_set_transform( const Mat4& transform )
{
    main_cam.transform = transform;
}

Mat4 RenderService::world_camera_get_projection() const
{
    return main_cam.projection;
}

Mat4 RenderService::world_get_view_matrix() const
{
    return view_matrix;
}

void RenderService::world_draw_instance( RID gpu_mesh, const Mat4& transform, RID material, uint32_t instancing )
{
    auto item        = ctx.world_render_list.acquire();
    item->gpu_mesh   = gpu_mesh;
    item->transform  = transform;
    item->material   = material;
    item->instancing = instancing;
}

void RenderService::canvas_draw_triangles(
    std::span<const Vertex2D> verts, std::span<const uint16_t> indices, RID material, Rect clip
)
{
    auto item      = ctx.canvas_render_list.acquire();
    item->material = material;
    item->clip     = clip;
    item->verts.assign( verts.begin(), verts.end() );
    item->indices.assign( indices.begin(), indices.end() );
}

void RenderService::canvas_draw_quad( Vec2 points[4], RID material, Color tint, Rect uv_rect, Rect clip )
{
    float u0 = uv_rect.x;
    float v0 = uv_rect.y;
    float u1 = uv_rect.w;
    float v1 = uv_rect.h;

    uint32_t c = static_cast<uint32_t>( tint.r ) | ( static_cast<uint32_t>( tint.g ) << 8 ) |
                 ( static_cast<uint32_t>( tint.b ) << 16 ) | ( static_cast<uint32_t>( tint.a ) << 24 );

    // clang-format off
    Vertex2D verts[4] = {
        { points[0].x, points[0].y, u0, v0, c },
        { points[1].x, points[1].y, u1, v0, c },
        { points[2].x, points[2].y, u1, v1, c },
        { points[3].x, points[3].y, u0, v1, c },
    };
    // clang-format on

    uint16_t indices[6] = { 0, 1, 2, 2, 3, 0 };

    canvas_draw_triangles( verts, indices, material, clip );
}

IRHI::Stats RenderService::get_stats() const
{
    return ctx.rhi->get_stats();
}

void RenderService::ensure_canvas_vb_( uint32_t needed )
{
    size_t required = needed * sizeof( Vertex2D );
    if (required <= canvas_vb_size * sizeof( Vertex2D ))
        return;

    uint32_t new_capacity = std::max( canvas_vb_size * 2, needed );
    BufferDesc desc;
    desc.debug_name  = "Canvas Vertex Buffer";
    desc.size        = new_capacity * sizeof( Vertex2D );
    desc.usage       = ResourceUsage::DYNAMIC;
    desc.access_mask = ResourceAccessFlags::WRITE;
    desc.bind_mask   = ResourceBindFlags::VERTEX_BUFFER;

    ctx.rhi->destroy_resource( canvas_vb );

    canvas_vb      = ctx.rhi->buffer_create( desc );
    canvas_vb_size = new_capacity;
}

void RenderService::ensure_canvas_ib_( uint32_t needed )
{
    size_t required = needed * sizeof( uint16_t );
    if (required <= canvas_ib_size * sizeof( uint16_t ))
        return;

    uint32_t new_capacity = std::max( canvas_ib_size * 2, needed );
    BufferDesc desc;
    desc.debug_name  = "Canvas Index Buffer";
    desc.size        = new_capacity * sizeof( uint16_t );
    desc.usage       = ResourceUsage::DYNAMIC;
    desc.access_mask = ResourceAccessFlags::WRITE;
    desc.bind_mask   = ResourceBindFlags::INDEX_BUFFER;

    ctx.rhi->destroy_resource( canvas_ib );

    canvas_ib      = ctx.rhi->buffer_create( desc );
    canvas_ib_size = new_capacity;
}

} // namespace nc
