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

    gfx_api = std::make_unique<DiligentRHI>();
    gfx_api->load_pso_cache();

    ctx.gfx_device_ctx = gfx_api->create_deferred_context( GpuQueue::GRAPHICS );

    storage.set_graphics_api( gfx_api.get() );

    return Error::OK;
}

void RenderService::shutdown()
{
    for (auto& sc : swapchains) {
        gfx_api->swapchain_destroy( sc );
    }
    swapchains.clear();

    if (canvas_vb.is_valid())
        gfx_api->destroy_rid( canvas_vb );
    if (canvas_ib.is_valid())
        gfx_api->destroy_rid( canvas_ib );

    storage.flush_pending_destroys();
    gfx_api->save_pso_cache();

    gfx_api.reset();
}

// ---------------------------------------------------------------------------

RID RenderService::swapchain_create(
    void* whnd, Vec2i size, rhi::TextureFormat color_format, rhi::TextureFormat depth_format
)
{
    rhi::SwapChainDesc desc{
        .native_whnd  = whnd,
        .initial_size = size,
        .is_primary   = true,
        .color_format = color_format,
        .depth_format = depth_format
    };
    RID rid = gfx_api->swapchain_create( desc );
    swapchains.push_back( rid );
    return rid;
}

void RenderService::swapchain_set_size( RID swapchain, Vec2i size )
{
    gfx_api->swapchain_set_size( swapchain, size );
}

Vec2i RenderService::swapchain_get_size( RID swapchain )
{
    return gfx_api->swapchain_get_size( swapchain );
}

RID RenderService::swapchain_get_primary() const
{
    return swapchains.empty() ? RID() : swapchains[0];
}

void RenderService::swapchain_destroy( RID swapchain )
{
    gfx_api->swapchain_destroy( swapchain );
    std::erase( swapchains, swapchain );
}

// ---------------------------------------------------------------------------

RID RenderService::texture_2d_create( const Image& image )
{
    rhi::TextureDesc desc{};
    desc.debug_name  = image.filepath;
    desc.format      = rhi::TextureFormat::RGBA8_UNORM_SRGB;
    desc.dimension   = rhi::ResourceDimension::DIM_2D;
    desc.usage       = rhi::ResourceUsage::DYNAMIC;
    desc.access_mask = rhi::ResourceAccessFlags::WRITE;
    desc.width       = image.get_width();
    desc.height      = image.get_height();
    desc.subresources.emplace_back( image.get_pixels().data() );
    return gfx_api->texture_create( desc );
}

RID RenderService::texture_cube_create( const CubeMap& cubemap )
{
    auto faces = cubemap.get_faces();

    rhi::TextureDesc desc;
    auto name       = std::format( "CubeTexture_{}_{}", cubemap.rid.value, cubemap.filepath );
    desc.debug_name = name;
    desc.format     = rhi::TextureFormat::RGBA8_UNORM_SRGB;
    desc.dimension  = rhi::ResourceDimension::DIM_CUBE;
    desc.usage      = rhi::ResourceUsage::IMMUTABLE;
    desc.width      = faces[0]->get_width();
    desc.height     = faces[0]->get_height();
    desc.array_size = 6;
    for (auto& face : faces) {
        desc.subresources.emplace_back( face->get_pixels().data() );
    }
    return gfx_api->texture_create( desc );
}

RID RenderService::texture_render_create( Vec2i size, rhi::TextureFormat format )
{
    rhi::ResourceBindFlags bind_mask = rhi::ResourceBindFlags::NONE;
    if (format == rhi::TextureFormat::D32_FLOAT) {
        bind_mask = rhi::ResourceBindFlags::DEPTH_STENCIL;
    } else {
        bind_mask = rhi::ResourceBindFlags::RENDER_TARGET | rhi::ResourceBindFlags::SHADER_RESOURCE;
    }

    rhi::TextureDesc desc{};
    desc.debug_name = "RenderTexture";
    desc.format     = format;
    desc.dimension  = rhi::ResourceDimension::DIM_2D;
    desc.usage      = rhi::ResourceUsage::DEFAULT;
    desc.bind_mask  = bind_mask;
    desc.width      = size.x;
    desc.height     = size.y;
    return gfx_api->texture_create( desc );
}

void* RenderService::texture_view_get( RID texture, rhi::TextureViewType view )
{
    return gfx_api->texture_view_get( texture, view );
}

void RenderService::texture_blit( RID tex_src, RID tex_dest )
{
    bool to_swapchain = !tex_dest;
    if (to_swapchain) {
        NC_FAIL_MSG_RET( swapchains.size() > 0, "Target texture is set to default (primary swapchain) but none exist" );
        tex_dest = swapchains[0];
    }
    gfx_api->texture_blit( tex_src, tex_dest, to_swapchain );
}

//------------------------------------------------------------------------------

RID RenderService::buffer_create( const rhi::BufferDesc& desc )
{
    return gfx_api->buffer_create( desc );
}

void RenderService::buffer_data_write( RID p_buffer, Span<const std::byte> p_src )
{
    gfx_api->buffer_data_write( p_buffer, p_src );
}

void RenderService::buffer_data_read( RID p_buffer, Span<std::byte> p_dst )
{
    gfx_api->buffer_data_read( p_buffer, p_dst );
}

void RenderService::buffer_blit( RID p_src_buffer, RID p_dst_buffer )
{
    gfx_api->buffer_blit( p_src_buffer, p_dst_buffer );
}

RID RenderService::resource_set_create(
    const Shader& p_shader, uint8_t p_set_idx, Span<const rhi::ResourceMappingEntry> p_resources
)
{
    return storage.resource_set_create( p_shader, p_set_idx, p_resources );
}

void RenderService::resource_set_bind( RID p_resource_set )
{
    storage.resource_set_bind( p_resource_set );
}

//------------------------------------------------------------------------------

RID RenderService::material_create( const MaterialTemplate& tmpl )
{
    return storage.material_create( tmpl );
}

void RenderService::material_set_texture( RID material, RID texture, uint32_t slot )
{
    storage.material_set_texture( material, texture, slot );
}

void RenderService::material_set_draw_mode( RID material, rhi::FillMode mode )
{
    storage.material_set_draw_mode( material, mode );
}

//------------------------------------------------------------------------------

RID RenderService::gpu_mesh_create( const Mesh& mesh )
{
    return storage.gpu_mesh_create( mesh );
}

//------------------------------------------------------------------------------

RID RenderService::compute_pipeline_create( const Shader& p_shader, Span<const RID> p_resource_sets )
{
    RenderStorage::PSOKey key;
    key.debug_name = p_shader.filepath + "_ComputePSO";
    key.cs         = gfx_api->shader_create(
        rhi::ShaderCreateDesc{
            .name     = p_shader.filepath + "_CS",
            .stage    = rhi::ShaderStage::COMPUTE,
            .bytecode = p_shader.get_bytecode( rhi::ShaderStage::COMPUTE )
        }
    );
    for (auto& e : p_resource_sets) {
        auto set = storage.resource_sets.get( e );
        NC_VERIFY( set );
        key.res_signatures.push_back( set->signature );
    }
    return storage.get_compute_pipeline_or_create( key );
}

void RenderService::compute_pipeline_bind( RID pipeline )
{
    gfx_api->set_queue( GpuQueue::COMPUTE );
    gfx_api->compute_pipeline_bind( pipeline );
}

void RenderService::compute_dispatch( uint32_t x, uint32_t y, uint32_t z )
{
    gfx_api->set_queue( GpuQueue::COMPUTE );
    gfx_api->compute_dispatch( x, y, z );
}

//------------------------------------------------------------------------------

RID RenderService::camera_create()
{
    return cameras.acquire();
}

RenderService::CameraAttribs& RenderService::camera_get_attribs( RID camera )
{
    auto cam = cameras.get( camera );
    NC_VERIFY( cam );
    return *cam;
}

Mat4 RenderService::camera_get_perspective( RID camera )
{
    auto& attribs = camera_get_attribs( camera );

    // perspective projection from target size
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix//building-basic-perspective-projection-matrix.html
    // https://github.com/DiligentGraphics/DiligentSamples/blob/master/SampleBase/src/SampleBase.cpp
    const auto aspect_ratio = static_cast<float>( attribs.DisplaySize.x ) / static_cast<float>( attribs.DisplaySize.y );
    const float y_scale     = 1.0f / std::tan( attribs.Fov * 0.5f );
    const float x_scale     = y_scale / aspect_ratio;
    const auto n            = attribs.zNear;
    const auto f            = attribs.zFar;
    const auto zz           = -f / ( f - n );     // used to remap z to [0,1]
    const auto wz           = -f * n / ( f - n ); // used to remap z [0,1]

    return Mat4(
        Vec4( x_scale, 0, 0, 0 ), // scale the x coordinates of the projected point
        Vec4( 0, y_scale, 0, 0 ), // scale the y coordinates of the projected point
        Vec4( 0, 0, zz, -1 ),     // set w = -z
        Vec4( 0, 0, wz, 0 )
    );
}

//------------------------------------------------------------------------------

bool RenderService::is_rid_owned( RID rid )
{
    return cameras.contains( rid ) || storage.is_rid_owned( rid );
}

bool RenderService::destroy_rid( RID rid )
{
    if (storage.destroy_rid( rid ))
        return true;
    if (gfx_api->destroy_rid( rid ))
        return true;
    return false;
}

// ---------------------------------------------------------------------------

void RenderService::render_begin( float delta_time )
{
    time += delta_time;
    last_dt_ = delta_time;

    gfx_api->set_queue( GpuQueue::GRAPHICS );
    gfx_api->set_context_state( false );
    gfx_api->begin_queries();
}

void RenderService::render_pass( const RenderPassDesc& desc )
{
    auto target_size = desc.target_rect.size();

    void* rtv = nullptr;
    void* dsv = nullptr;

    if (desc.to_screen) {
        auto primary = swapchain_get_primary();
        NC_ASSERT( primary, "No primary swapchain exist to render onto" );
        rtv         = gfx_api->swapchain_get_view( primary, rhi::TextureViewType::RENDER_TARGET );
        dsv         = gfx_api->swapchain_get_view( primary, rhi::TextureViewType::DEPTH_STENCIL );
        target_size = gfx_api->swapchain_get_size( primary );
    } else if (desc.color_target && gfx_api->is_rid_owned( desc.color_target )) {
        rtv = gfx_api->texture_view_get( desc.color_target, rhi::TextureViewType::RENDER_TARGET );
        if (desc.depth_target && gfx_api->is_rid_owned( desc.depth_target ))
            dsv = gfx_api->texture_view_get( desc.depth_target, rhi::TextureViewType::DEPTH_STENCIL );
    }

    if (!rtv)
        return;

    NC_LOG_TRACE_C(
        log::GRAPHICS, "render_pass: size={}x{} rtv={} dsv={}", target_size.x, target_size.y,
        reinterpret_cast<uintptr_t>( rtv ), reinterpret_cast<uintptr_t>( dsv )
    );

    const void* rtvs[] = { rtv };
    Rect2i full_rect( desc.target_rect.x, desc.target_rect.y, target_size.x, target_size.y );
    IRHI::Viewport vp{
        .rect = Rect2f(
            static_cast<float>( full_rect.x ), static_cast<float>( full_rect.y ), static_cast<float>( full_rect.w ),
            static_cast<float>( full_rect.h )
        )
    };

    gfx_api->render_target_bind( rtvs, dsv );
    gfx_api->render_target_set_scissor_rect( { &full_rect, 1 } );
    gfx_api->render_target_set_viewport( { &vp, 1 } );

    if (desc.clear) {
        gfx_api->render_target_clear_color( rtv, desc.clear_color );
        if (dsv)
            gfx_api->render_target_clear_depth( dsv );
    }

    RenderStorage::ShaderConstants constants;
    constants.Time      = time;
    constants.DeltaTime = last_dt_;

    if (desc.draw_spatial && desc.camera) {
        auto& cam_attribs = camera_get_attribs( desc.camera );
        // precompute the V*P part of M*V*P so we don't have do it on the GPU.
        // here we take the inverse of camera transform to get its tex_view_type matrix.
        auto view_matrix         = cam_attribs.Transform.affine_inverse();
        constants.CameraMatrix   = cam_attribs.Transform;
        constants.ViewProjMatrix = camera_get_perspective( desc.camera ) * view_matrix;

        for (auto& item : ctx.world_render_list) {
            NC_ASSERT( item.material.is_valid(), "A valid material is required to draw a spatial renderable." );
            NC_LOG_TRACE_C(
                log::GRAPHICS, "world_items_flush: gpu_mesh_rid={} instances={}", item.gpu_mesh.value, item.instancing
            );
            auto mesh                = storage.get_gpu_mesh( item.gpu_mesh );
            constants.ModelMatrix    = item.transform;
            constants.ModelMatrixInv = item.transform.affine_inverse();
            storage.material_bind( item.material, constants );
            storage.gpu_mesh_bind( item.gpu_mesh );
            gfx_api->draw_indexed( mesh->index_count, 0, 0, item.instancing );
        }
    }

    if (desc.draw_canvas) {
        constants.ModelMatrix    = Mat4::identity();
        constants.ModelMatrixInv = Mat4::identity();
        // clang-format off
		// ortho projection for canvas items.
		auto sx = static_cast<float>( target_size.x );
		auto sy = static_cast<float>( target_size.y );
		constants.ViewProjMatrix = Mat4(
		    Vec4(  2.0f / sx,  0.0f,      0.0f,  0.0f ),
		    Vec4(  0.0f,      -2.0f / sy, 0.0f,  0.0f ),
		    Vec4(  0.0f,       0.0f,      1.0f,  0.0f ),
		    Vec4( -1.0f,       1.0f,      0.0f,  1.0f )
		);
        // clang-format on

        NC_LOG_TRACE_C( log::GRAPHICS, "render_pass: canvas_render_list={}", ctx.canvas_render_list.size() );
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

            gfx_api->buffer_data_write(
                canvas_vb, { reinterpret_cast<std::byte*>( item.verts.data() ), item.verts.size() * sizeof( Vertex2D ) }
            );
            gfx_api->buffer_data_write(
                canvas_ib, { reinterpret_cast<std::byte*>( item.indices.data() ), item.indices.size() * sizeof( uint16_t ) }
            );

            storage.material_set_texture( item.material, item.texture, 0 );
            storage.material_bind( item.material, constants );

            gfx_api->buffer_vertices_bind( { &canvas_vb, 1 }, 0 );
            gfx_api->buffer_index_bind( canvas_ib, 0 );

            if (item.clip.x >= 0 && item.clip.y >= 0 && item.clip.w > 0 && item.clip.h > 0) {
                gfx_api->render_target_set_scissor_rect( { &item.clip, 1 } );
            }

            NC_LOG_TRACE_C( log::GRAPHICS, "canvas_items_flush: draw indexed (indices={})", item.indices.size() );
            gfx_api->draw_indexed( static_cast<uint32_t>( item.indices.size() ), 0, 0 );
        }
    }
}

void RenderService::present()
{
    gfx_api->end_queries();
    gfx_api->swapchain_present( swapchains[0], settings.VSync );
    storage.flush_pending_destroys();
    ctx.world_render_list.reset();
    ctx.canvas_render_list.release_all(); // CanvasRenderItem is non-POD
}

// ---------------------------------------------------------------------------

void RenderService::spatial_draw_instance( RID gpu_mesh, const Mat4& transform, RID material, uint32_t instancing )
{
    auto item        = ctx.world_render_list.acquire();
    item->gpu_mesh   = gpu_mesh;
    item->transform  = transform;
    item->material   = material;
    item->instancing = instancing;
}

void RenderService::canvas_draw_triangles(
    Span<const Vertex2D> verts, Span<const uint16_t> indices, RID material, RID texture, Rect2i clip
)
{
    auto item      = ctx.canvas_render_list.acquire();
    item->material = material;
    item->texture  = texture;
    item->clip     = clip;
    item->verts.assign( verts.begin(), verts.end() );
    item->indices.assign( indices.begin(), indices.end() );
}

void RenderService::canvas_draw_quad(
    Vec2f points[4], RID material, RID texture, Color tint, Rect2i uv_rect, Rect2i clip
)
{
    float u0 = static_cast<float>( uv_rect.x );
    float v0 = static_cast<float>( uv_rect.y );
    float u1 = static_cast<float>( uv_rect.w );
    float v1 = static_cast<float>( uv_rect.h );

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

    canvas_draw_triangles( verts, indices, material, texture, clip );
}

IRHI::Stats RenderService::get_stats() const
{
    return gfx_api->get_stats();
}

// ---------------------------------------------------------------------------

void RenderService::ensure_canvas_vb_( uint32_t needed )
{
    size_t required = needed * sizeof( Vertex2D );
    if (required <= canvas_vb_size * sizeof( Vertex2D ))
        return;

    uint32_t new_capacity = std::max( canvas_vb_size * 2, needed );
    rhi::BufferDesc desc;
    desc.debug_name  = "Canvas Vertex Buffer";
    desc.size        = new_capacity * sizeof( Vertex2D );
    desc.usage       = rhi::ResourceUsage::DYNAMIC;
    desc.access_mask = rhi::ResourceAccessFlags::WRITE;
    desc.bind_mask   = rhi::ResourceBindFlags::VERTEX_BUFFER;

    gfx_api->destroy_rid( canvas_vb );

    canvas_vb      = gfx_api->buffer_create( desc );
    canvas_vb_size = new_capacity;
}

void RenderService::ensure_canvas_ib_( uint32_t needed )
{
    size_t required = needed * sizeof( uint16_t );
    if (required <= canvas_ib_size * sizeof( uint16_t ))
        return;

    uint32_t new_capacity = std::max( canvas_ib_size * 2, needed );
    rhi::BufferDesc desc;
    desc.debug_name  = "Canvas Index Buffer";
    desc.size        = new_capacity * sizeof( uint16_t );
    desc.usage       = rhi::ResourceUsage::DYNAMIC;
    desc.access_mask = rhi::ResourceAccessFlags::WRITE;
    desc.bind_mask   = rhi::ResourceBindFlags::INDEX_BUFFER;

    gfx_api->destroy_rid( canvas_ib );

    canvas_ib      = gfx_api->buffer_create( desc );
    canvas_ib_size = new_capacity;
}

} // namespace nc
