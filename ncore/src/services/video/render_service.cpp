#if defined( _WIN32 )
#define NOMINMAX
#endif

#include <algorithm>
#include <bit>
#include <cmath>

#include <backends/diligent/gfx_diligent.h>

#include <ncore/core/matrix.h>
#include <ncore/core/rect.h>
#include <ncore/core/vector.h>
#include <ncore/resources/resource.h>
#include <ncore/services/video/render_service.h>
#include <ncore/services/video/renderer/vertex_format.h>
#include <ncore/utils/config.h>
#include <ncore/utils/log.h>

namespace nc {

Error RenderService::init( ConfFile& cfg_file )
{
    settings = cfg_file.read<RenderSettings>();

    gfx_api = std::make_unique<DiligentGfxImpl>();
    gfx_api->load_pso_cache();

    gfx_device_ctx = gfx_api->create_deferred_context( GpuQueue::GRAPHICS );

    storage.set_graphics_api( gfx_api.get() );

    return Error::OK;
}

void RenderService::shutdown()
{
    for (auto& sc : swapchains) {
        gfx_api->swapchain_destroy( sc );
    }
    swapchains.clear();

    if (canvas_vbo.is_valid())
        gfx_api->destroy_rid( canvas_vbo );
    if (canvas_ibo.is_valid())
        gfx_api->destroy_rid( canvas_ibo );

    storage.flush_pending_destroys();
    gfx_api->save_pso_cache();

    gfx_api.reset();
}

// ---------------------------------------------------------------------------

RID RenderService::swapchain_create(
    void* whnd, Vec2i size, gfx::TextureFormat color_format, gfx::TextureFormat depth_format
)
{
    gfx::SwapChainDesc desc{
        .native_whnd  = whnd,
        .initial_size = size,
        .is_primary   = true,
        .color_format = color_format,
        .depth_format = depth_format
    };
    RID rid = gfx_api->swapchain_create( desc );
    swapchains.insert( rid );
    if (!primary_swapchain)
        primary_swapchain = rid;
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
    return primary_swapchain;
}

void RenderService::swapchain_destroy( RID swapchain )
{
    gfx_api->swapchain_destroy( swapchain );
    swapchains.erase( swapchain );
    if (primary_swapchain == swapchain)
        primary_swapchain = 0;
}

// ---------------------------------------------------------------------------

RID RenderService::texture_2d_create( const Image& image )
{
    gfx::TextureDesc desc{};
    desc.debug_name      = image.filepath;
    desc.format          = gfx::TextureFormat::RGBA8_UNORM_SRGB;
    desc.dimension       = gfx::ResourceDimension::DIM_2D;
    desc.usage           = gfx::ResourceUsage::DYNAMIC;
    desc.cpu_access_mask = gfx::ResourceCpuAccessFlags::WRITE;
    desc.width           = image.get_width();
    desc.height          = image.get_height();
    desc.subresources.emplace_back( image.get_pixels().data() );
    return gfx_api->texture_create( desc );
}

RID RenderService::texture_cube_create( const CubeMap& cubemap )
{
    gfx::TextureDesc desc;
    auto name       = std::format( "CubeTexture_{}_{}", cubemap.rid.value, cubemap.filepath );
    desc.debug_name = name;
    desc.format     = gfx::TextureFormat::RGBA8_UNORM_SRGB;
    desc.dimension  = gfx::ResourceDimension::DIM_CUBE;
    desc.usage      = gfx::ResourceUsage::IMMUTABLE;
    desc.width      = cubemap.get_face_size();
    desc.height     = cubemap.get_face_size();
    desc.array_size = 6;
    for (uint32_t face = 0; face < 6; ++face) {
        desc.subresources.emplace_back( cubemap.get_face_data( face ).data() );
    }
    return gfx_api->texture_create( desc );
}

RID RenderService::texture_render_create( Vec2i size, gfx::TextureFormat format )
{
    gfx::ResourceBindFlags bind_mask = gfx::ResourceBindFlags::NONE;
    if (format == gfx::TextureFormat::D32_FLOAT) {
        // Sampleable depth: the transparent pass reads the opaque depth as an SRV.
        bind_mask = gfx::ResourceBindFlags::DEPTH_STENCIL | gfx::ResourceBindFlags::SHADER_RESOURCE;
    } else {
        bind_mask = gfx::ResourceBindFlags::RENDER_TARGET | gfx::ResourceBindFlags::SHADER_RESOURCE;
    }

    gfx::TextureDesc desc{};
    desc.debug_name = "RenderTexture";
    desc.format     = format;
    desc.dimension  = gfx::ResourceDimension::DIM_2D;
    desc.usage      = gfx::ResourceUsage::DEFAULT;
    desc.bind_mask  = bind_mask;
    desc.width      = size.x;
    desc.height     = size.y;
    return gfx_api->texture_create( desc );
}

void* RenderService::texture_view_get( RID texture, gfx::TextureViewType view )
{
    return gfx_api->texture_view_get( texture, view );
}

void RenderService::texture_blit( RID tex_src, RID tex_dest )
{
    bool to_swapchain = !tex_dest;
    if (to_swapchain) {
        NC_FAIL_MSG_RET(
            primary_swapchain, "Target texture is set to default (primary swapchain) but it does not exist"
        );
        tex_dest = primary_swapchain;
    }
    gfx_api->texture_blit( tex_src, tex_dest, to_swapchain );
}

//------------------------------------------------------------------------------

RID RenderService::buffer_create( const gfx::BufferDesc& desc )
{
    return gfx_api->buffer_create( desc );
}

void RenderService::buffer_data_write( RID p_buffer, const void* p_src, size_t p_src_size )
{
    gfx_api->buffer_data_write( p_buffer, p_src, p_src_size );
}

void RenderService::buffer_data_read( RID p_buffer, void* p_dst, size_t p_dst_size )
{
    gfx_api->buffer_data_read( p_buffer, p_dst, p_dst_size );
}

void RenderService::buffer_blit( RID p_src_buffer, RID p_dst_buffer )
{
    gfx_api->buffer_blit( p_src_buffer, p_dst_buffer );
}

RID RenderService::resource_set_create(
    const Shader& p_shader, uint8_t p_set_idx, Span<const gfx::ResourceMappingEntry> p_resources
)
{
    return storage.resource_set_create( p_shader, p_set_idx, p_resources );
}

void RenderService::resource_set_bind( RID p_resource_set )
{
    storage.resource_set_bind( p_resource_set );
}

//------------------------------------------------------------------------------

RID RenderService::material_create( const Ref<MaterialShader>& p_shader )
{
    return storage.material_create( p_shader );
}

void RenderService::material_set_param( RID p_material, const String& p_name, const void* p_data, size_t p_data_size )
{
    storage.material_set_param( p_material, p_name, p_data, p_data_size );
}

void RenderService::material_set_texture( RID material, RID texture, uint32_t slot )
{
    storage.material_set_texture( material, texture, slot );
}

void RenderService::material_set_draw_mode( RID material, gfx::FillMode mode )
{
    storage.material_set_draw_mode( material, mode );
}

//------------------------------------------------------------------------------

RID RenderService::mesh_create( const Mesh& mesh )
{
    return storage.mesh_create( mesh );
}

//------------------------------------------------------------------------------

RID RenderService::compute_pipeline_create( const Shader& p_shader, Span<const RID> p_resource_sets )
{
    RenderStorage::PSOKey key;
    key.debug_name = "ComputePSO_" + p_shader.filepath;
    key.cs         = gfx_api->shader_create(
        gfx::ShaderCreateDesc{
            .name     = "ComputeShader_" + p_shader.filepath,
            .stage    = gfx::ShaderStage::COMPUTE,
            .bytecode = p_shader.get_bytecode( gfx::ShaderStage::COMPUTE )
        }
    );
    for (auto& e : p_resource_sets) {
        auto set = storage.resource_sets.get_or_fail( e );
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

RenderService::CameraAttribs* RenderService::camera_attribs_get( RID camera )
{
    auto cam = cameras.get( camera );
    return cam;
}

Mat4 RenderService::camera_perspective_get( RID camera )
{
    return camera_perspective_get_( *camera_attribs_get( camera ) );
}

// ---------------------------------------------------------------------------

RID RenderService::spatial_item_create()
{
    return ctx.spatial_items.acquire();
}

void RenderService::spatial_item_set_mesh( RID p_spatial_item, RID p_gpu_mesh )
{
    auto item      = ctx.spatial_items.get_or_fail( p_spatial_item );
    item->gpu_mesh = p_gpu_mesh;
}

void RenderService::spatial_item_set_transform( RID p_spatial_item, const Mat4& p_transform )
{
    auto item       = ctx.spatial_items.get_or_fail( p_spatial_item );
    item->transform = p_transform;
}

void RenderService::spatial_item_set_material( RID p_spatial_item, RID p_material )
{
    auto item      = ctx.spatial_items.get_or_fail( p_spatial_item );
    item->material = p_material;
}

void RenderService::spatial_item_draw( RID p_spatial_item, uint32_t instancing )
{
    auto item = ctx.spatial_items.get_or_fail( p_spatial_item );
    // alloc() hands out raw storage — emplace() runs the constructors so the
    // remaining fields are not whatever was in the arena
    auto cmd = ctx.spatial_draw_cmds.emplace();
    if (!cmd) {
        NC_LOG_ERROR_C( log::GRAPHICS, "spatial_item_draw: spatial draw command arena is full, draw skipped" );
        return;
    }
    cmd->item        = item;
    cmd->instancing  = instancing;
    cmd->index_count = storage.mesh_get_index_count( item->gpu_mesh );
    // RID is (validator << 32) | index; the index is the stable slot id, so use only
    // the low 32 bits of each handle. Packing the full 64-bit mesh value into the key
    // would overlap the material bits via OR and break the sort order.
    uint64_t mat_key  = ( item->material.value & 0xFFFFFFFFull ) << 32;
    uint64_t mesh_key = ( item->gpu_mesh.value & 0xFFFFFFFFull );
    cmd->sort_key     = mat_key | mesh_key;
}

// ---------------------------------------------------------------------------

RID RenderService::canvas_item_create()
{
    return ctx.canvas_items.acquire();
}

void RenderService::canvas_item_set_material( RID p_canvas_item, RID p_material )
{
    auto item      = ctx.canvas_items.get_or_fail( p_canvas_item );
    item->material = p_material;
}

void RenderService::canvas_item_set_clipping( RID p_canvas_item, Rect2i clip )
{
    auto item  = ctx.canvas_items.get_or_fail( p_canvas_item );
    item->clip = clip;
}

void RenderService::canvas_item_add_triangles(
    RID p_canvas_item, Span<const Vertex2D> verts, Span<const uint16_t> indices
)
{
    auto item = ctx.canvas_items.get_or_fail( p_canvas_item );
    item->verts.assign( verts.begin(), verts.end() );
    item->indices.assign( indices.begin(), indices.end() );
}

void RenderService::canvas_item_add_quad(
    RID p_canvas_item, Vec2f p_points[4], Rect2i p_uv_rect, Color p_tint, RID p_texture
)
{
    float u0 = static_cast<float>( p_uv_rect.x );
    float v0 = static_cast<float>( p_uv_rect.y );
    float u1 = static_cast<float>( p_uv_rect.w );
    float v1 = static_cast<float>( p_uv_rect.h );

    uint32_t c = static_cast<uint32_t>( p_tint.r ) | ( static_cast<uint32_t>( p_tint.g ) << 8 ) |
                 ( static_cast<uint32_t>( p_tint.b ) << 16 ) | ( static_cast<uint32_t>( p_tint.a ) << 24 );

    Vertex2D vertices[4] = {
        { p_points[0].x, p_points[0].y, u0, v0, c },
        { p_points[1].x, p_points[1].y, u1, v0, c },
        { p_points[2].x, p_points[2].y, u1, v1, c },
        { p_points[3].x, p_points[3].y, u0, v1, c }
    };

    uint16_t indices[6] = { 0, 1, 2, 2, 3, 0 };

    auto item     = ctx.canvas_items.get_or_fail( p_canvas_item );
    item->texture = p_texture;
    canvas_item_add_triangles( p_canvas_item, vertices, indices );
}

void RenderService::canvas_item_draw( RID p_canvas_item, uint32_t z_order )
{
    auto item = ctx.canvas_items.get_or_fail( p_canvas_item );
    auto cmd  = ctx.canvas_draw_cmds.emplace();
    if (!cmd) {
        NC_LOG_ERROR_C( log::GRAPHICS, "canvas_item_draw: canvas draw command arena is full, draw skipped" );
        return;
    }
    cmd->item             = item;
    cmd->idx_count        = static_cast<uint32_t>( item->indices.size() );
    cmd->texture_override = item->texture;
    cmd->z_order          = z_order;

    ctx.canvas_staging_vert_count += static_cast<uint32_t>( item->verts.size() );
    ctx.canvas_staging_idx_count += static_cast<uint32_t>( item->indices.size() );
}

void RenderService::canvas_item_draw(
    RID p_canvas_item, Span<const Vertex2D> p_verts_override, Span<const uint16_t> p_indices_override,
    RID p_texture_override, Rect2i p_clip_override, uint32_t z_order
)
{
    auto item = ctx.canvas_items.get_or_fail( p_canvas_item );
    auto cmd  = ctx.canvas_draw_cmds.emplace();
    if (!cmd) {
        NC_LOG_ERROR_C( log::GRAPHICS, "canvas_item_draw: canvas draw command arena is full, draw skipped" );
        return;
    }
    cmd->item             = item;
    cmd->idx_count        = static_cast<uint32_t>( p_indices_override.size() );
    cmd->verts_override   = p_verts_override;
    cmd->indices_override = p_indices_override;
    cmd->texture_override = p_texture_override;
    cmd->clip_override    = p_clip_override;
    cmd->z_order          = z_order;

    ctx.canvas_override_vert_count += static_cast<uint32_t>( p_verts_override.size() );
    ctx.canvas_override_idx_count += static_cast<uint32_t>( p_indices_override.size() );
}

// ---------------------------------------------------------------------------

void RenderService::prepare_frame( float delta_time )
{
    time += delta_time;
    last_dt_ = delta_time;
    storage.next_frame();
    m_stats.pass_count = 0;

    // any compute work submitted this frame must be visible to the
    // graphics queue before we record draws.
    gfx_api->queue_submit_and_wait( GpuQueue::COMPUTE, GpuQueue::GRAPHICS );

    gfx_api->set_queue( GpuQueue::GRAPHICS );
    gfx_api->set_context_state( false );
    gfx_api->begin_general_queries();
}

void RenderService::render_target( const RenderTargetDesc& p_desc )
{
    auto target_size = p_desc.rectangle.size();

    void* rtv = nullptr;
    void* dsv = nullptr;

    if (p_desc.to_screen) {
        auto primary = swapchain_get_primary();
        NC_ASSERT_MSG( primary, "No primary swapchain exist to render onto" );
        rtv         = gfx_api->swapchain_get_view( primary, gfx::TextureViewType::RENDER_TARGET );
        dsv         = gfx_api->swapchain_get_view( primary, gfx::TextureViewType::DEPTH_STENCIL );
        target_size = gfx_api->swapchain_get_size( primary );
    } else if (p_desc.color_texture && gfx_api->is_rid_owned( p_desc.color_texture )) {
        rtv = gfx_api->texture_view_get( p_desc.color_texture, gfx::TextureViewType::RENDER_TARGET );
        if (p_desc.depth_texture && gfx_api->is_rid_owned( p_desc.depth_texture ))
            dsv = gfx_api->texture_view_get( p_desc.depth_texture, gfx::TextureViewType::DEPTH_STENCIL );
    }

    if (!rtv) {
        NC_LOG_TRACE_C(
            log::GRAPHICS,
            "render_frame SKIPPED: to_screen={} color={} depth={} camera={} draw_spatial={} draw_canvas={}",
            p_desc.to_screen, p_desc.color_texture.value, p_desc.depth_texture.value, p_desc.camera.value,
            p_desc.draw_spatial, p_desc.draw_canvas
        );
        return;
    }

    const uint32_t pass_slot = m_stats.pass_count;
    if (pass_slot < GfxInterface::MAX_TIMESTAMP_SLOTS)
        gfx_api->timestamp_begin( pass_slot );

    ctx.pass_has_depth = ( dsv != nullptr );

    NC_LOG_TRACE_C(
        log::GRAPHICS, "render_frame: size={}x{} rtv={} dsv={}", target_size.x, target_size.y,
        reinterpret_cast<uintptr_t>( rtv ), reinterpret_cast<uintptr_t>( dsv )
    );

    const void* rtvs[] = { rtv };
    Rect2i full_rect( p_desc.rectangle.x, p_desc.rectangle.y, target_size.x, target_size.y );
    GfxInterface::Viewport vp{
        .rect = Rect2f(
            static_cast<float>( full_rect.x ), static_cast<float>( full_rect.y ), static_cast<float>( full_rect.w ),
            static_cast<float>( full_rect.h )
        )
    };

    // The shared global SRB may still reference the previous frame's depth as
    // `g_SceneDepth`. Committing it while that same texture is bound as the depth
    // attachment would transition it to DEPTH_READ mid-pass, so point it at the
    // fallback first; the transparent pass rebinds the real depth after the opaque
    // batch. (No-op when the fallback is already bound.)
    storage.scene_depth_set( RID() );

    gfx_api->render_target_bind( rtvs, dsv );
    gfx_api->render_target_set_scissor_rect( { &full_rect, 1 } );
    gfx_api->render_target_set_viewport( { &vp, 1 } );

    if (p_desc.clear) {
        gfx_api->render_target_clear_color( rtv, p_desc.clear_color );
        if (dsv)
            gfx_api->render_target_clear_depth( dsv );
    }

    bool should_draw_spatial = p_desc.draw_spatial && p_desc.camera;
    bool should_draw_canvas  = p_desc.draw_canvas;

    RenderStorage::SceneData scene_data;
    scene_data.render_width  = static_cast<uint32_t>( target_size.x );
    scene_data.render_height = static_cast<uint32_t>( target_size.y );
    scene_data.time          = time;
    scene_data.delta_time    = last_dt_;

    CameraAttribs* cam_attribs = nullptr;
    if (should_draw_spatial) {
        cam_attribs              = camera_attribs_get( p_desc.camera );
        scene_data.z_near        = cam_attribs->zNear;
        scene_data.z_far         = cam_attribs->zFar;
        scene_data.view_matrix   = cam_attribs->Transform.affine_inverse();
        scene_data.camera_matrix = cam_attribs->Transform;
        // precompute the V*P part of M*V*P so we don't have do it on the GPU.
        // here we take the inverse of camera transform to get its view matrix.
        scene_data.view_proj_matrix = camera_perspective_get_( *cam_attribs ) * scene_data.view_matrix;
    }

    storage.scene_data_update( scene_data );

    if (should_draw_spatial) {
        auto& opaque_idx      = ctx.spatial_sort_indices;
        auto& transparent_idx = ctx.spatial_transparent_sort_indices;
        opaque_idx.clear();
        transparent_idx.clear();
        for (size_t i = 0; i < ctx.spatial_draw_cmds.head(); ++i) {
            auto* cmd = ctx.spatial_draw_cmds[i];
            if (storage.material_is_transparent( cmd->item->material ))
                transparent_idx.push_back( static_cast<uint32_t>( i ) );
            else
                opaque_idx.push_back( static_cast<uint32_t>( i ) );
        }

        // Opaque first, grouped by material then mesh so state changes are minimized.
        std::sort( opaque_idx.begin(), opaque_idx.end(), [&]( uint32_t a, uint32_t b ) {
            return ctx.spatial_draw_cmds[a]->sort_key < ctx.spatial_draw_cmds[b]->sort_key;
        } );

        // Transparent draws blend against whatever is already in the target, so they run
        // after the opaque pass and are sorted back-to-front by distance to the camera.
        const float cam_x         = cam_attribs->Transform.col3.x;
        const float cam_y         = cam_attribs->Transform.col3.y;
        const float cam_z         = cam_attribs->Transform.col3.z;
        const auto dist_to_cam_sq = [&]( const Mat4& m ) {
            const float dx = m.col3.x - cam_x;
            const float dy = m.col3.y - cam_y;
            const float dz = m.col3.z - cam_z;
            return dx * dx + dy * dy + dz * dz;
        };
        std::sort( transparent_idx.begin(), transparent_idx.end(), [&]( uint32_t a, uint32_t b ) {
            return dist_to_cam_sq( ctx.spatial_draw_cmds[a]->item->transform ) >
                   dist_to_cam_sq( ctx.spatial_draw_cmds[b]->item->transform );
        } );

        const auto draw_spatial_cmd = [&]( uint32_t idx ) {
            auto* cmd      = ctx.spatial_draw_cmds[idx];
            auto& material = cmd->item->material;
            auto gpu_mesh  = cmd->item->gpu_mesh;

            storage.material_bind( material, ctx.pass_has_depth );
            storage.mesh_bind( gpu_mesh );

            SpatialInstanceData instance{};
            instance.model_matrix  = cmd->item->transform;
            instance.normal_matrix = cmd->item->transform.affine_inverse();
            gfx_api->set_inline_constants(
                storage.spatial_push_binding, "g_Instance", &instance, 0, sizeof( instance ) / 4
            );
            gfx_api->resource_binding_commit( storage.spatial_push_binding );

            gfx_api->draw_indexed( cmd->index_count, 0, 0, cmd->instancing, 0 );
        };

        for (uint32_t idx : opaque_idx)
            draw_spatial_cmd( idx );

        if (!transparent_idx.empty()) {
            // Transparent materials sample the opaque depth as an SRV, which is illegal
            // while it is the bound depth attachment. Unbind the depth (keeping color and
            // its contents), expose it as `g_SceneDepth`, and draw the transparent batch
            // without a depth attachment — water performs its own depth test in-shader.
            storage.scene_depth_set( p_desc.depth_texture );
            gfx_api->render_target_bind( rtvs, nullptr );
            gfx_api->render_target_set_scissor_rect( { &full_rect, 1 } );
            gfx_api->render_target_set_viewport( { &vp, 1 } );
            ctx.pass_has_depth = false;

            for (uint32_t idx : transparent_idx)
                draw_spatial_cmd( idx );
        }
    }

    if (should_draw_canvas) {
        uint32_t total_verts = ctx.canvas_override_vert_count + ctx.canvas_staging_vert_count;
        uint32_t total_ids   = ctx.canvas_override_idx_count + ctx.canvas_staging_idx_count;

        if (total_verts > 0 && total_ids > 0) {
            ensure_canvas_vertex_buf_( total_verts );
            ensure_canvas_index_buf_( total_ids );

            // one contiguous staging pass followed by a single DISCARD upload. Partial
            // writes would need Map() per region, and the first DISCARD would throw away
            // everything already written in this frame.
            ctx.canvas_verts_staging.resize( total_verts );
            ctx.canvas_indices_staging.resize( total_ids );

            uint32_t vert_off = 0;
            uint32_t idx_off  = 0;

            for (size_t i = 0; i < ctx.canvas_draw_cmds.head(); ++i) {
                auto* cmd = ctx.canvas_draw_cmds[i];
                const auto v =
                    cmd->verts_override.size() > 0 ? cmd->verts_override : Span<const Vertex2D>( cmd->item->verts );
                const auto ix   = cmd->indices_override.size() > 0 ? cmd->indices_override
                                                                   : Span<const uint16_t>( cmd->item->indices );
                cmd->start_vert = vert_off;
                cmd->start_idx  = idx_off;

                if (!v.empty()) {
                    std::memcpy( ctx.canvas_verts_staging.data() + vert_off, v.data(), v.size() * sizeof( Vertex2D ) );
                    vert_off += static_cast<uint32_t>( v.size() );
                }
                if (!ix.empty()) {
                    std::memcpy(
                        ctx.canvas_indices_staging.data() + idx_off, ix.data(), ix.size() * sizeof( uint16_t )
                    );
                    idx_off += static_cast<uint32_t>( ix.size() );
                }
            }

            if (vert_off > 0) {
                gfx_api->buffer_data_write(
                    canvas_vbo, ctx.canvas_verts_staging.data(), static_cast<size_t>( vert_off ) * sizeof( Vertex2D )
                );
            }
            if (idx_off > 0) {
                gfx_api->buffer_data_write(
                    canvas_ibo, ctx.canvas_indices_staging.data(), static_cast<size_t>( idx_off ) * sizeof( uint16_t )
                );
            }

            gfx_api->buffer_vertices_bind( { &canvas_vbo, 1 }, 0 );
            gfx_api->buffer_index_bind( canvas_ibo, 0 );

            // Sort canvas draws by z-order. A stable sort preserves submission order
            // within the same z, which is the correct blend order for non-depth-tested
            // alpha-blended geometry.
            auto& sort_idx = ctx.canvas_sort_indices;
            sort_idx.clear();
            for (size_t i = 0; i < ctx.canvas_draw_cmds.head(); ++i)
                sort_idx.push_back( static_cast<uint32_t>( i ) );
            std::stable_sort( sort_idx.begin(), sort_idx.end(), [&]( uint32_t a, uint32_t b ) {
                return ctx.canvas_draw_cmds[a]->z_order < ctx.canvas_draw_cmds[b]->z_order;
            } );

            // The 2D ortho projection lives in a push constant now. The data persists on
            // the SRB, so set it once and only re-commit after each PSO bind.
            const float sx = static_cast<float>( target_size.x );
            const float sy = static_cast<float>( target_size.y );
            CanvasInstanceData instance;
            instance.view_proj_matrix = Mat4(
                Vec4f( 2.0f / sx, 0.0f, 0.0f, 0.0f ), Vec4f( 0.0f, -2.0f / sy, 0.0f, 0.0f ),
                Vec4f( 0.0f, 0.0f, 1.0f, 0.0f ), Vec4f( -1.0f, 1.0f, 0.0f, 1.0f )
            );
            constexpr uint32_t kCanvasPushUints = static_cast<uint32_t>( sizeof( CanvasInstanceData ) / 4 );
            gfx_api->set_inline_constants( storage.canvas_push_binding, "g_Instance", &instance, 0, kCanvasPushUints );

            // draw
            for (uint32_t idx : sort_idx) {
                auto* cmd = ctx.canvas_draw_cmds[idx];
                if (cmd->idx_count == 0)
                    continue;

                RID material = cmd->item->material;
                if (cmd->texture_override)
                    material = storage.material_create_texture_variant( material, cmd->texture_override );

                storage.material_bind( material, ctx.pass_has_depth );

                if (storage.material_has_push_constants( material ))
                    gfx_api->resource_binding_commit( storage.canvas_push_binding );

                auto clip = cmd->clip_override;
                if (clip.w <= 0 || clip.h <= 0)
                    clip = cmd->item->clip;

                if (clip.x >= 0 && clip.y >= 0 && clip.w > 0 && clip.h > 0)
                    gfx_api->render_target_set_scissor_rect( { &clip, 1 } );

                gfx_api->draw_indexed( cmd->idx_count, cmd->start_idx, cmd->start_vert );
            }

            gfx_api->render_target_set_scissor_rect( { &full_rect, 1 } );
        }
    }

    if (pass_slot < RenderStats::kMaxPassTimestamps) {
        const double ms = gfx_api->timestamp_end( pass_slot );
        if (ms >= 0.0)
            m_stats.pass_duration_ms[pass_slot] = ms;
    }
    ++m_stats.pass_count;
}

void RenderService::present()
{
    gfx_api->end_general_queries();
    gfx_api->swapchain_present( swapchain_get_primary(), settings.VSync );
    storage.flush_pending_destroys();
    ctx.clear();
}

//------------------------------------------------------------------------------

bool RenderService::is_rid_owned( RID rid )
{
    return cameras.contains( rid ) || storage.is_rid_owned( rid );
}

bool RenderService::destroy_rid( RID rid )
{
    if (cameras.release( rid ))
        return true;
    if (storage.destroy_rid( rid ))
        return true;
    if (gfx_api->destroy_rid( rid ))
        return true;
    return false;
}

// ---------------------------------------------------------------------------

void RenderService::ensure_canvas_vertex_buf_( uint32_t p_needed_verts )
{
    size_t required = p_needed_verts * sizeof( Vertex2D );
    if (required <= canvas_vbo_size * sizeof( Vertex2D ))
        return;

    // Grow to the next power of two to reduce how often the buffer is recreated.
    uint32_t new_capacity = canvas_vbo_size ? canvas_vbo_size : 1;
    while (new_capacity < p_needed_verts)
        new_capacity <<= 1;
    gfx::BufferDesc desc;
    desc.debug_name      = "Canvas Vertex Buffer";
    desc.size            = new_capacity * sizeof( Vertex2D );
    desc.usage           = gfx::ResourceUsage::DYNAMIC;
    desc.cpu_access_mask = gfx::ResourceCpuAccessFlags::WRITE;
    desc.bind_mask       = gfx::ResourceBindFlags::VERTEX_BUFFER;

    gfx_api->destroy_rid( canvas_vbo );

    canvas_vbo      = gfx_api->buffer_create( desc );
    canvas_vbo_size = new_capacity;
}

void RenderService::ensure_canvas_index_buf_( uint32_t p_needed_ids )
{
    size_t required = p_needed_ids * sizeof( uint16_t );
    if (required <= canvas_ibo_size * sizeof( uint16_t ))
        return;

    // Grow to the next power of two to reduce how often the buffer is recreated.
    uint32_t new_capacity = canvas_ibo_size ? canvas_ibo_size : 1;
    while (new_capacity < p_needed_ids)
        new_capacity <<= 1;
    gfx::BufferDesc desc;
    desc.debug_name      = "Canvas Index Buffer";
    desc.size            = new_capacity * sizeof( uint16_t );
    desc.usage           = gfx::ResourceUsage::DYNAMIC;
    desc.cpu_access_mask = gfx::ResourceCpuAccessFlags::WRITE;
    desc.bind_mask       = gfx::ResourceBindFlags::INDEX_BUFFER;

    gfx_api->destroy_rid( canvas_ibo );

    canvas_ibo      = gfx_api->buffer_create( desc );
    canvas_ibo_size = new_capacity;
}

Mat4 RenderService::camera_perspective_get_( CameraAttribs& attribs )
{
    // Reuse the cached matrix unless a projection input changed. Compare floats bitwise
    // so we recompute exactly when a value changes, without tripping -Wfloat-equal.
    const bool same_fov  = std::bit_cast<uint32_t>( attribs.cached_fov ) == std::bit_cast<uint32_t>( attribs.Fov );
    const bool same_near = std::bit_cast<uint32_t>( attribs.cached_znear ) == std::bit_cast<uint32_t>( attribs.zNear );
    const bool same_far  = std::bit_cast<uint32_t>( attribs.cached_zfar ) == std::bit_cast<uint32_t>( attribs.zFar );
    if (same_fov && same_near && same_far && attribs.cached_display == attribs.DisplaySize)
        return attribs.cached_proj;

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

    attribs.cached_proj = Mat4(
        Vec4f( x_scale, 0, 0, 0 ), // scale the x coordinates of the projected point
        Vec4f( 0, y_scale, 0, 0 ), // scale the y coordinates of the projected point
        Vec4f( 0, 0, zz, -1 ),     // set w = -z
        Vec4f( 0, 0, wz, 0 )
    );
    attribs.cached_fov     = attribs.Fov;
    attribs.cached_znear   = attribs.zNear;
    attribs.cached_zfar    = attribs.zFar;
    attribs.cached_display = attribs.DisplaySize;

    return attribs.cached_proj;
}

} // namespace nc
