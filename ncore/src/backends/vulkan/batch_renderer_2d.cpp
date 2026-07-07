#include "batch_renderer_2d.h"

#include <BasicMath.hpp>
#include <GraphicsTypes.h>
#include <MapHelper.hpp>
#include <cstring>

#include <ncore/kernel/structures.h>

namespace nc {

static void append_vertex( std::vector<uint8_t>& buf, float X, float Y, float u, float v, uint32_t color )
{
    Vertex2D vert{ X, Y, u, v, color };
    auto* p = reinterpret_cast<const uint8_t*>( &vert );
    buf.insert( buf.end(), p, p + sizeof( Vertex2D ) );
}

static void add_quad(
    std::vector<uint8_t>& verts, std::vector<uint16_t>& idx, float x1, float y1, float x2, float y2, float u1, float v1,
    float u2, float v2, uint32_t color
)
{
    uint16_t base = static_cast<uint16_t>( verts.size() / sizeof( Vertex2D ) );
    append_vertex( verts, x1, y1, u1, v1, color );
    append_vertex( verts, x2, y1, u2, v1, color );
    append_vertex( verts, x2, y2, u2, v2, color );
    append_vertex( verts, x1, y2, u1, v2, color );
    idx.insert(
        idx.end(), { base, static_cast<uint16_t>( base + 1 ), static_cast<uint16_t>( base + 2 ), base,
                     static_cast<uint16_t>( base + 2 ), static_cast<uint16_t>( base + 3 ) }
    );
}

static uint32_t color_to_u32( const Color& c )
{
    return ( static_cast<uint32_t>( c.r ) ) | ( static_cast<uint32_t>( c.g ) << 8 ) |
           ( static_cast<uint32_t>( c.b ) << 16 ) | ( static_cast<uint32_t>( c.a ) << 24 );
}

BatchRenderer2D::BatchRenderer2D(
    DiligentRef<Diligent::IRenderDevice> device, DiligentRef<Diligent::IDeviceContext> ctx,
    DiligentRef<Diligent::IPipelineState> pso, DiligentRef<Diligent::IShaderResourceBinding> srb,
    Diligent::IShaderResourceVariable* tex_var
) : render_device( device ), device_ctx( ctx ), m_pso( pso ), m_srb( srb ), m_texture_var( tex_var )
{}

void BatchRenderer2D::push_quad( void* native_texture, Vec4 dest, Vec4 src, Color tint )
{
    BatchDrawCmd cmd;
    cmd.native_texture = native_texture;
    cmd.is_textured    = true;

    uint32_t col = color_to_u32( tint );

    float u1 = src.X, v1 = src.Y, u2 = src.w, v2 = src.h;
    float x1 = dest.X, y1 = dest.Y, x2 = dest.X + dest.w, y2 = dest.Y + dest.h;

    add_quad( cmd.vertices, cmd.indices, x1, y1, x2, y2, u1, v1, u2, v2, col );

    m_cmds.push_back( std::move( cmd ) );
}

void BatchRenderer2D::push_indexed(
    const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count, void* native_texture,
    Vec4 clip_rect
)
{
    BatchDrawCmd cmd;
    cmd.native_texture = native_texture;
    cmd.clip_rect      = clip_rect;
    cmd.is_textured    = true;

    auto* v = static_cast<const uint8_t*>( vertices );
    cmd.vertices.assign( v, v + vertex_count * VERTEX_STRIDE );
    cmd.indices.assign( indices, indices + index_count );

    m_cmds.push_back( std::move( cmd ) );
}

void BatchRenderer2D::flush( DiligentRef<Diligent::IBuffer> constants, Vec2 surf_size )
{
    if (m_cmds.empty())
        return;

    // Upload projection matrix
    {
        float L = 0.0f, R = surf_size.X;
        float T = 0.0f, B = surf_size.Y;

        Diligent::float4x4 proj{
            2.0f / ( R - L ),
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            2.0f / ( T - B ),
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.5f,
            0.0f,
            ( R + L ) / ( L - R ),
            ( T + B ) / ( B - T ),
            0.5f,
            1.0f
        };

        Diligent::MapHelper<Diligent::float4x4> cb{
            device_ctx, constants, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD
        };
        if (cb)
            *cb = proj;
    }

    // Compute total buffer requirements
    size_t total_vb_bytes = 0, total_ib = 0;
    for (auto& cmd : m_cmds) {
        total_vb_bytes += cmd.vertices.size();
        total_ib += cmd.indices.size();
    }

    auto needed_vb = static_cast<Diligent::Uint32>( total_vb_bytes / VERTEX_STRIDE );
    auto needed_ib = static_cast<Diligent::Uint32>( total_ib );
    ensure_buffers_( needed_vb, needed_ib );

    // Merge all draws into VB/IB
    {
        Diligent::MapHelper<uint8_t> vb_map{ device_ctx, m_vb, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD };
        Diligent::MapHelper<uint8_t> ib_map{ device_ctx, m_ib, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD };
        if (!vb_map || !ib_map)
            return;

        uint8_t* vb_ptr = vb_map;
        uint8_t* ib_ptr = ib_map;
        for (auto& cmd : m_cmds) {
            memcpy( vb_ptr, cmd.vertices.data(), cmd.vertices.size() );
            vb_ptr += cmd.vertices.size();
            memcpy( ib_ptr, cmd.indices.data(), cmd.indices.size() * sizeof( uint16_t ) );
            ib_ptr += cmd.indices.size() * sizeof( uint16_t );
        }
    }

    // Draw
    Diligent::Uint32 vb_offset = 0;
    Diligent::Uint32 ib_offset = 0;

    for (auto& cmd : m_cmds) {
        if (cmd.indices.empty()) {
            continue;
        }

        Diligent::IBuffer* vb[] = { m_vb };
        device_ctx->SetVertexBuffers(
            0, 1, vb, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
            Diligent::SET_VERTEX_BUFFERS_FLAG_RESET
        );
        device_ctx->SetIndexBuffer( m_ib, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
        device_ctx->SetPipelineState( m_pso );

        const float bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        device_ctx->SetBlendFactors( bf );

        // Scissor
        if (!cmd.clip_rect.is_zero()) {
            Diligent::Rect scissor{
                static_cast<Diligent::Int32>( cmd.clip_rect.X ), static_cast<Diligent::Int32>( cmd.clip_rect.Y ),
                static_cast<Diligent::Int32>( cmd.clip_rect.X + cmd.clip_rect.w ),
                static_cast<Diligent::Int32>( cmd.clip_rect.Y + cmd.clip_rect.h )
            };
            scissor.left   = std::max( scissor.left, 0 );
            scissor.top    = std::max( scissor.top, 0 );
            scissor.right  = std::min( scissor.right, static_cast<Diligent::Int32>( surf_size.X ) );
            scissor.bottom = std::min( scissor.bottom, static_cast<Diligent::Int32>( surf_size.Y ) );
            if (scissor.IsValid())
                device_ctx->SetScissorRects(
                    1, &scissor, static_cast<Diligent::Uint32>( surf_size.X ),
                    static_cast<Diligent::Uint32>( surf_size.Y )
                );
        } else {
            Diligent::Rect full{
                0, 0, static_cast<Diligent::Int32>( surf_size.X ), static_cast<Diligent::Int32>( surf_size.Y )
            };
            device_ctx->SetScissorRects(
                1, &full, static_cast<Diligent::Uint32>( surf_size.X ), static_cast<Diligent::Uint32>( surf_size.Y )
            );
        }

        auto* tex_view = static_cast<Diligent::ITextureView*>( cmd.native_texture );
        if (m_texture_var && tex_view)
            m_texture_var->Set( tex_view );
        device_ctx->CommitShaderResources( m_srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

        Diligent::DrawIndexedAttribs attrs{
            static_cast<Diligent::Uint32>( cmd.indices.size() ), Diligent::VT_UINT16, Diligent::DRAW_FLAG_NONE
        };
        attrs.FirstIndexLocation = ib_offset;
        attrs.BaseVertex         = vb_offset;
        device_ctx->DrawIndexed( attrs );

        vb_offset += static_cast<Diligent::Uint32>( cmd.vertices.size() / VERTEX_STRIDE );
        ib_offset += static_cast<Diligent::Uint32>( cmd.indices.size() );
    }

    m_cmds.clear();
}

void BatchRenderer2D::ensure_buffers_( size_t needed_vb, size_t needed_ib )
{
    auto nv = static_cast<Diligent::Uint32>( needed_vb );
    auto ni = static_cast<Diligent::Uint32>( needed_ib );

    if (nv > m_vb_capacity) {
        m_vb.Release();
        while (m_vb_capacity < nv)
            m_vb_capacity = m_vb_capacity == 0 ? 1024 : m_vb_capacity * 2;
        Diligent::BufferDesc bd;
        bd.Size           = m_vb_capacity * VERTEX_STRIDE;
        bd.Usage          = Diligent::USAGE_DYNAMIC;
        bd.BindFlags      = Diligent::BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        render_device->CreateBuffer( bd, nullptr, &m_vb );
    }
    if (ni > m_ib_capacity) {
        m_ib.Release();
        while (m_ib_capacity < ni)
            m_ib_capacity = m_ib_capacity == 0 ? 2048 : m_ib_capacity * 2;
        Diligent::BufferDesc bd;
        bd.Size           = m_ib_capacity * sizeof( uint16_t );
        bd.Usage          = Diligent::USAGE_DYNAMIC;
        bd.BindFlags      = Diligent::BIND_INDEX_BUFFER;
        bd.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        render_device->CreateBuffer( bd, nullptr, &m_ib );
    }
}

} // namespace nc
