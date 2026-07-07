#include <cfloat>
#include <cmath>
#include <numbers>

#include <ncore/modules/video/graphics_module.h>
#include <ncore/modules/video/resources/image.h>
#include <ncore/modules/video/resources/material.h>
#include <ncore/modules/video/resources/mesh.h>
#include <ncore/modules/video/resources/shader.h>

namespace nc {

GraphicsModule::GraphicsModule( std::unique_ptr<IRenderBackend> backend ) : renderer( std::move( backend ) ) {}

Error GraphicsModule::init()
{
    auto err = renderer->init();
    if (err != Error::OK)
        return err;
    white_texture = create_white_texture_();
    return Error::OK;
}

void GraphicsModule::finalize()
{
    renderer->finalize();
}

RID GraphicsModule::create_white_texture_()
{
    const uint32_t white = 0xFFFFFFFF;
    return renderer->create_texture( 1, 1, &white );
}

void GraphicsModule::begin_frame()
{
    renderer->begin_frame();
}

void GraphicsModule::end_frame()
{
    renderer->batch_2d_flush();
    renderer->end_frame();
}

void GraphicsModule::set_clear_color( Color color )
{
    renderer->set_clear_color( color );
}

void GraphicsModule::set_vsync( bool enabled )
{
    renderer->set_vsync( enabled );
}

void GraphicsModule::set_render_size( Vec2 size )
{
    renderer->set_render_size( size );
}

Vec2 GraphicsModule::get_surface_size() const
{
    return renderer->get_surface_size();
}

RID GraphicsModule::upload_texture( const Image& image )
{
    return renderer->create_texture(
        static_cast<uint32_t>( image.get_width() ), static_cast<uint32_t>( image.get_height() ),
        image.get_pixels().data()
    );
}

RID GraphicsModule::upload_pipeline( const Material& material )
{
    std::string vs_spirv, ps_spirv;

    for (auto& shader : material.get_shaders()) {
        auto src = shader.get_source();
        if (src.empty())
            continue;
        if (vs_spirv.empty())
            vs_spirv = src;
        else if (ps_spirv.empty())
            ps_spirv = src;
    }

    if (vs_spirv.empty() || ps_spirv.empty())
        return RID();

    return renderer->create_pipeline( vs_spirv, ps_spirv );
}

RID GraphicsModule::upload_mesh( const Mesh& mesh )
{
    auto vertices = mesh.get_vertices();
    auto indices  = mesh.get_indices();

    if (vertices.empty() || indices.empty())
        return RID();

    RID vb = renderer->create_buffer( BufferType::Vertex, vertices.size(), vertices.data(), false );
    RID ib = renderer->create_buffer( BufferType::Index, indices.size(), indices.data(), false );
    ( void ) ib;

    // TODO: finish this

    return vb;
}

void GraphicsModule::destroy_resource( RID rid )
{
    renderer->destroy_resource( rid );
}

void GraphicsModule::fill_rect( Vec4 rect, Color color )
{
    renderer->batch_push_quad( white_texture, rect, Vec4( 0, 0, 1, 1 ), color );
}

void GraphicsModule::draw_rect( Vec4 rect, Color color, float thickness )
{
    float x1 = rect.X, y1 = rect.Y;
    float x2 = rect.X + rect.w, y2 = rect.Y + rect.h;
    float t = thickness;

    renderer->batch_push_quad( white_texture, Vec4( x1, y1, x2 - x1, t ), Vec4(), color );
    renderer->batch_push_quad( white_texture, Vec4( x1, y2 - t, x2 - x1, t ), Vec4(), color );
    renderer->batch_push_quad( white_texture, Vec4( x1, y1, t, y2 - y1 ), Vec4(), color );
    renderer->batch_push_quad( white_texture, Vec4( x2 - t, y1, t, y2 - y1 ), Vec4(), color );
}

void GraphicsModule::draw_line( Vec2 from, Vec2 to, Color color, float thickness )
{
    float dx  = to.X - from.X;
    float dy  = to.Y - from.Y;
    float len = std::sqrt( dx * dx + dy * dy );
    if (len < 0.001f)
        return;

    float nx = -dy / len * thickness * 0.5f;
    float ny = dx / len * thickness * 0.5f;

    std::vector<uint8_t> verts;
    verts.reserve( 4 * sizeof( Vertex2D ) );

    auto push = [&]( float X, float Y ) {
        Vertex2D v{ X, Y, 0.0f, 0.0f, 0 };
        auto* p = reinterpret_cast<const uint8_t*>( &v );
        verts.insert( verts.end(), p, p + sizeof( Vertex2D ) );
    };

    push( from.X + nx, from.Y + ny );
    push( to.X + nx, to.Y + ny );
    push( to.X - nx, to.Y - ny );
    push( from.X - nx, from.Y - ny );

    std::vector<uint16_t> indices = { 0, 1, 2, 0, 2, 3 };

    renderer->batch_push_indexed(
        verts.data(), 4, indices.data(), static_cast<uint32_t>( indices.size() ), white_texture, Vec4()
    );
}

void GraphicsModule::draw_point( Vec2 pos, Color color )
{
    renderer->batch_push_quad( white_texture, Vec4( pos.X, pos.Y, 1.0f, 1.0f ), Vec4(), color );
}

void GraphicsModule::draw_textured_quad( RID texture, Vec4 dest, Vec4 src, Color tint )
{
    renderer->batch_push_quad( texture, dest, src, tint );
}

void GraphicsModule::draw_indexed(
    const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count, RID texture,
    Vec4 clip_rect
)
{
    renderer->batch_push_indexed( vertices, vertex_count, indices, index_count, texture, clip_rect );
}

void* GraphicsModule::get_native_device() const
{
    return renderer->get_native_device();
}

void* GraphicsModule::get_native_handle( RID rid ) const
{
    return renderer->get_native_texture_view( rid );
}

void GraphicsModule::draw_circle( float x, float y, int radius, Color color, bool filled, bool edge )
{
    if (filled) {
        for (int w = -radius; w <= radius; w++) {
            for (int h = -radius; h <= radius; h++) {
                if (w * w + h * h <= radius * radius) {
                    draw_point(
                        Vec2(
                            static_cast<float>( x + static_cast<float>( w ) ),
                            static_cast<float>( y + static_cast<float>( h ) )
                        ),
                        color
                    );
                }
            }
        }

        if (edge) {
            for (int i = 0; i < 360; i++) {
                float angle = static_cast<float>( i ) * std::numbers::pi_v<float> / 180.0f;
                int xc      = static_cast<int>( x ) + ( radius + 2 ) * static_cast<int>( std::cos( angle ) );
                int ys      = static_cast<int>( y ) + ( radius + 2 ) * static_cast<int>( std::sin( angle ) );
                draw_point( Vec2( static_cast<float>( xc ), static_cast<float>( ys ) ), Color( 255, 255, 255, 150 ) );
            }
        }
    } else {
        for (int i = 0; i < 360; i++) {
            float angle = static_cast<float>( i ) * std::numbers::pi_v<float> / 180.0f;
            int xc      = static_cast<int>( x ) + radius * static_cast<int>( std::cos( angle ) );
            int ys      = static_cast<int>( y ) + radius * static_cast<int>( std::sin( angle ) );
            draw_point( Vec2( static_cast<float>( xc ), static_cast<float>( ys ) ), color );
        }
    }
}

void GraphicsModule::draw_convex_polygon_filled( const Vec2* vertices, int count, const Color& color )
{
    float minY = vertices[0].Y, maxY = vertices[0].Y;
    for (int i = 1; i < count; i++) {
        if (vertices[i].Y < minY)
            minY = vertices[i].Y;
        if (vertices[i].Y > maxY)
            maxY = vertices[i].Y;
    }

    int yMin = static_cast<int>( minY );
    int yMax = static_cast<int>( maxY );

    for (int Y = yMin; Y <= yMax; Y++) {
        float rowStart = FLT_MAX, rowEnd = -FLT_MAX;
        float yf = static_cast<float>( Y );

        for (int i = 0, j = count - 1; i < count; j = i++) {
            float y1 = vertices[j].Y, y2 = vertices[i].Y;
            if (( y1 <= yf && y2 > yf ) || ( y2 <= yf && y1 > yf )) {
                float t = ( yf - y1 ) / ( y2 - y1 );
                float X = vertices[j].X + t * ( vertices[i].X - vertices[j].X );
                if (X < rowStart)
                    rowStart = X;
                if (X > rowEnd)
                    rowEnd = X;
            }
        }

        if (rowEnd > rowStart) {
            draw_line( Vec2( rowStart, yf ), Vec2( rowEnd, yf ), color, 1.0f );
        }
    }
}

} // namespace nc
