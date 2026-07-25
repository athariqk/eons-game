#if defined( _WIN32 )
#define NOMINMAX
#endif

#include <backends/diligent/rhi_diligent.h>

#include <ncore/modules/video/render_module.h>
#include <ncore/modules/video/renderer/geometry.h>
#include <ncore/resources/image.h>
#include <ncore/resources/material_template.h>
#include <ncore/resources/mesh.h>
#include <ncore/resources/shader.h>
#include <ncore/utils/config.h>
#include <ncore/utils/log.h>

namespace nc {

Error RenderModule::init( ConfFile& cfg_file )
{
    settings = cfg_file.read<RenderSettings>();

    ctx.rhi = new DiligentRHI();
    ctx.rhi->load_pso_cache();

    ctx.gfx_device_ctx = ctx.rhi->create_deferred_context( IRHI::GpuQueue::Graphics );

    return Error::OK;
}

void RenderModule::shutdown()
{
    ctx.rhi->save_pso_cache();
    delete ctx.rhi;
}

RID RenderModule::swapchain_create( void* whnd, Vec2 size )
{
    RID rid =
        ctx.rhi->swapchain_create( SwapChainDesc{ .native_whnd = whnd, .initial_size = size, .is_primary = true } );
    swapchains.push_back( rid );
    return rid;
}

void RenderModule::swapchain_set_size( RID sc, Vec2 size )
{
    ctx.rhi->swapchain_set_size( sc, size );
}

Vec2 RenderModule::swapchain_get_size( RID sc )
{
    return ctx.rhi->swapchain_get_size( sc );
}

void RenderModule::swapchain_destroy( RID sc )
{
    ctx.rhi->swapchain_destroy( sc );
    std::erase( swapchains, sc );
}

RID RenderModule::material_create( const MaterialTemplate& tmpl )
{
    return ctx.storage.material_create( tmpl, ctx.rhi );
}

void RenderModule::material_set_texture( RID material, RID texture, uint32_t slot )
{
    Material* mat = ctx.storage.get_material( material );
    if (!mat || slot >= mat->texture_slots.size())
        return;

    auto& ts = mat->texture_slots[slot];
    if (ts.srb_index < mat->srbs.size() && mat->srbs[ts.srb_index].is_valid())
        ctx.rhi->texture_binding_update( texture, mat->srbs[ts.srb_index], ts.name.c_str() );
}

RID RenderModule::create_texture_2d( const Image& image )
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
            .pixels      = image.get_pixels().data()
        }
    );
}

void RenderModule::destroy_rid( RID rid )
{
    ctx.rhi->destroy_resource( rid );
}

void RenderModule::frame_begin()
{
    ctx.rhi->set_queue( IRHI::GpuQueue::Graphics );
    ctx.rhi->set_context_state( false );
    // ctx.rhi->set_context_state( true, ctx.rhi_ctx );
    // ctx.rhi->commands_record_begin();
    auto rtv = ctx.rhi->swapchain_get_view( swapchains[0], TextureViewType::RENDER_TARGET );
    auto dsv = ctx.rhi->swapchain_get_view( swapchains[0], TextureViewType::DEPTH_STENCIL );
    NC_LOG_TRACE_C(
        log::GRAPHICS, "frame_end: rtv={} dsv={}", reinterpret_cast<uintptr_t>( rtv ),
        reinterpret_cast<uintptr_t>( dsv )
    );

    auto size = ctx.rhi->swapchain_get_size( swapchains[0] );

    ortho_proj[0]  = 2.0f / size.X;
    ortho_proj[1]  = 0.0f;
    ortho_proj[2]  = 0.0f;
    ortho_proj[3]  = 0.0f;
    ortho_proj[4]  = 0.0f;
    ortho_proj[5]  = -2.0f / size.Y;
    ortho_proj[6]  = 0.0f;
    ortho_proj[7]  = 0.0f;
    ortho_proj[8]  = 0.0f;
    ortho_proj[9]  = 0.0f;
    ortho_proj[10] = 1.0f;
    ortho_proj[11] = 0.0f;
    ortho_proj[12] = -1.0f;
    ortho_proj[13] = 1.0f;
    ortho_proj[14] = 0.0f;
    ortho_proj[15] = 1.0f;

    IRHI::Viewport vd{};
    vd.rect.X = 0.0f;
    vd.rect.Y = 0.0f;
    vd.rect.W = size.X;
    vd.rect.H = size.Y;
    ctx.rhi->render_target_set_viewport( { &vd, 1 } );

    const void* rtvs[] = { rtv };
    ctx.rhi->render_target_bind( rtvs, dsv );
    ctx.rhi->render_target_clear_color( rtv, Color( 128, 128, 128, 255 ) ); // TODO: don't hardcode grey
    ctx.rhi->render_target_clear_depth( dsv );
}

void RenderModule::frame_end()
{
    NC_LOG_TRACE_C( log::GRAPHICS, "frame_end: canvas_render_list={}", ctx.canvas_render_list.size() );

    for (auto& item : ctx.canvas_render_list) {
        if (item.verts.empty()) {
            NC_LOG_TRACE_C( log::GRAPHICS, "flush_canvas_: skipped (empty)" );
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

        Material* mat = ctx.storage.get_material( item.material );
        NC_ASSERT_NULL( mat );

        NC_LOG_TRACE_C(
            log::GRAPHICS, "canvas_items_flush: PSO rid={} CB rid={} SRBs={}", mat->pso.value,
            mat->constant_buffer.value, mat->srbs.size()
        );

        ctx.rhi->gfx_pipeline_bind( mat->pso );

        uint64_t vb_offset = 0;
        ctx.rhi->vertex_buffers_bind( { &canvas_vb, 1 }, 0, { &vb_offset, 1 } );
        ctx.rhi->index_buffer_bind( canvas_ib, 0 );

        ctx.rhi->buffer_update( mat->constant_buffer, ortho_proj, sizeof( ortho_proj ) );
        NC_LOG_TRACE_C(
            log::GRAPHICS, "canvas_items_flush: CB updated ({} bytes, [{:.4f}, {:.4f}, {:.4f}, {:.4f} ...])",
            sizeof( ortho_proj ), ortho_proj[0], ortho_proj[1], ortho_proj[4], ortho_proj[5]
        );

        for (auto& srb : mat->srbs) {
            if (srb.is_valid())
                ctx.rhi->resource_binding_commit( srb );
        }

        if (item.clip.W > 0 && item.clip.H > 0) {
            ctx.rhi->render_target_set_scissor_rect( { &item.clip, 1 } );
        }

        NC_LOG_TRACE_C( log::GRAPHICS, "canvas_items_flush: draw indexed (indices={})", item.indices.size() );
        ctx.rhi->draw_indexed( static_cast<uint32_t>( item.indices.size() ), 0, 0, 1 );
    }

    ctx.rhi->swapchain_present( swapchains[0], settings.VSync );

    // ctx.rhi->commands_release();

    ctx.canvas_render_list.clear();
}

void RenderModule::canvas_draw_triangles(
    std::span<const Vertex2D> verts, std::span<const uint16_t> indices, RID material, Vec4 clip
)
{
    auto& item    = ctx.canvas_render_list.emplace_back();
    item.material = material;
    item.clip     = clip;
    item.verts.assign( verts.begin(), verts.end() );
    item.indices.assign( indices.begin(), indices.end() );
}

void RenderModule::canvas_draw_quad( Vec2 pos, Vec2 size, RID material, Vec4 uv_rect, Color tint, Vec4 clip )
{
    float x0 = pos.X;
    float y0 = pos.Y;
    float x1 = pos.X + size.X;
    float y1 = pos.Y + size.Y;

    float u0 = uv_rect.X;
    float v0 = uv_rect.Y;
    float u1 = uv_rect.H;
    float v1 = uv_rect.W;

    uint32_t c = static_cast<uint32_t>( tint.r ) | ( static_cast<uint32_t>( tint.g ) << 8 ) |
                 ( static_cast<uint32_t>( tint.b ) << 16 ) | ( static_cast<uint32_t>( tint.a ) << 24 );

    // clang-format off
    Vertex2D verts[4] = {
        { x0, y0, u0, v0, c },
        { x1, y0, u1, v0, c },
        { x1, y1, u1, v1, c },
        { x0, y1, u0, v1, c },
    };
    // clang-format on

    uint16_t indices[6] = { 0, 1, 2, 2, 3, 0 };

    canvas_draw_triangles( verts, indices, material, clip );
}

void RenderModule::ensure_canvas_vb_( uint32_t needed )
{
    size_t required = needed * sizeof( Vertex2D );
    if (required <= canvas_vb_size * sizeof( Vertex2D ))
        return;

    uint32_t new_capacity = std::max( canvas_vb_size * 2, needed );
    BufferDesc desc;
    desc.size        = new_capacity * sizeof( Vertex2D );
    desc.usage       = ResourceUsage::DYNAMIC;
    desc.access_mask = ResourceAccessFlags::WRITE;
    desc.bind_mask   = ResourceBindFlags::VERTEX_BUFFER;

    ctx.rhi->destroy_resource( canvas_vb );

    canvas_vb      = ctx.rhi->buffer_create( desc );
    canvas_vb_size = new_capacity;
}

void RenderModule::ensure_canvas_ib_( uint32_t needed )
{
    size_t required = needed * sizeof( uint16_t );
    if (required <= canvas_ib_size * sizeof( uint16_t ))
        return;

    uint32_t new_capacity = std::max( canvas_ib_size * 2, needed );
    BufferDesc desc;
    desc.size        = new_capacity * sizeof( uint16_t );
    desc.usage       = ResourceUsage::DYNAMIC;
    desc.access_mask = ResourceAccessFlags::WRITE;
    desc.bind_mask   = ResourceBindFlags::INDEX_BUFFER;

    ctx.rhi->destroy_resource( canvas_ib );

    canvas_ib      = ctx.rhi->buffer_create( desc );
    canvas_ib_size = new_capacity;
}

} // namespace nc
