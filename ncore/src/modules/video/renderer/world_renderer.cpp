#include <algorithm>

#include <ncore/modules/video/renderer/world_renderer.h>

namespace nc {

WorldRenderer::WorldRenderer()
{
    const size_t pool_size = 256;

    BufferDesc desc;
    desc.size        = pool_size * sizeof( float ) * 16;
    desc.usage       = ResourceUsage::DYNAMIC;
    desc.access_mask = ResourceAccessFlags::WRITE;
    desc.bind_mask   = ResourceBindFlags::UNIFORM_BUFFER;

    for (size_t i = 0; i < pool_size; ++i)
        model_ubo_pool.push_back( m_rhi->buffer_create( desc ) );
}

void WorldRenderer::submit_mesh(
    RID vertex_buffer, RID index_buffer, uint32_t index_count, RID pso, RID srb, const Mat4& /*model_matrix*/
)
{
    if (ubo_index >= model_ubo_pool.size())
        return;

    RID ubo = model_ubo_pool[ubo_index++];

    // Sort key: material (PSO) first, then texture/SRB
    uint64_t key = static_cast<uint64_t>( pso.value );

    draw_queue.push_back( { vertex_buffer, index_buffer, index_count, 0, pso, srb, ubo, key } );
}

void WorldRenderer::flush()
{
    if (draw_queue.empty())
        return;

    std::sort( draw_queue.begin(), draw_queue.end(), []( const WorldDrawCmd& a, const WorldDrawCmd& b ) {
        return a.sort_key < b.sort_key;
    } );

    RID current_pso;
    RID current_srb;

    for (auto& cmd : draw_queue) {
        if (cmd.pso != current_pso) {
            current_pso = cmd.pso;
            m_rhi->gfx_pipeline_bind( current_pso );
        }

        if (cmd.srb != current_srb) {
            current_srb = cmd.srb;
        }

        m_rhi->resource_binding_commit( cmd.srb );

        RID vb             = cmd.vertex_buffer;
        uint64_t vb_offset = 0;
        m_rhi->vertex_buffers_bind( { &vb, 1 }, 0, { &vb_offset, 1 } );
        m_rhi->index_buffer_bind( cmd.index_buffer, 0 );

        m_rhi->draw_indexed( cmd.index_count, cmd.first_index, 0, 1 );
    }
}

void WorldRenderer::clear()
{
    draw_queue.clear();
    ubo_index = 0;
}

RID WorldRenderer::allocate_model_ubo()
{
    if (ubo_index < model_ubo_pool.size())
        return model_ubo_pool[ubo_index++];
    return {};
}

} // namespace nc
