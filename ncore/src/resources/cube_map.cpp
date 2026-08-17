#include <ncore/resources/cube_map.h>
#include <ncore/resources/image.h>

namespace nc {

struct RGBA8 {
    uint8_t channels[4];
};

static RGBA8 bilinear_sample_( const uint8_t* pixels, int w, int h, float eu, float ev )
{
    float fx = eu * static_cast<float>( w ) - 0.5f;
    float fy = ev * static_cast<float>( h ) - 0.5f;

    int x0   = static_cast<int>( std::floor( fx ) );
    int y0   = static_cast<int>( std::floor( fy ) );
    float tx = fx - std::floor( fx );
    float ty = fy - std::floor( fy );

    auto wrapped = [w]( int x ) { return ( ( x % w ) + w ) % w; };
    int x1       = wrapped( x0 + 1 );
    int y1       = std::clamp( y0 + 1, 0, h - 1 );
    x0           = wrapped( x0 );
    y0           = std::clamp( y0, 0, h - 1 );

    const uint8_t* p00 = pixels + ( static_cast<size_t>( y0 ) * w + x0 ) * 4;
    const uint8_t* p10 = pixels + ( static_cast<size_t>( y0 ) * w + x1 ) * 4;
    const uint8_t* p01 = pixels + ( static_cast<size_t>( y1 ) * w + x0 ) * 4;
    const uint8_t* p11 = pixels + ( static_cast<size_t>( y1 ) * w + x1 ) * 4;

    RGBA8 out;
    for (int c = 0; c < 4; c++) {
        float top       = p00[c] * ( 1.0f - tx ) + p10[c] * tx;
        float bottom    = p01[c] * ( 1.0f - tx ) + p11[c] * tx;
        out.channels[c] = static_cast<uint8_t>( top * ( 1.0f - ty ) + bottom * ty );
    }
    return out;
}

static Vec3 face_direction_( int face, float u, float v )
{
    switch (face) {
        case 0:
            return Vec3( 1.0f, v, -u );  // +X
        case 1:
            return Vec3( -1.0f, v, u );  // -X
        case 2:
            return Vec3( u, 1.0f, -v );  // +Y
        case 3:
            return Vec3( u, -1.0f, v );  // -Y
        case 4:
            return Vec3( u, v, 1.0f );   // +Z
        default:
            return Vec3( -u, v, -1.0f ); // -Z
    }
}

CubeMap::CubeMap( const Ref<Image>& equirect, uint32_t face_size )
{
    const int ew   = static_cast<int>( equirect->get_width() );
    const int eh   = static_cast<int>( equirect->get_height() );
    const auto src = reinterpret_cast<const uint8_t*>( equirect->get_pixels().data() );

    if (face_size <= 0) {
        face_size = equirect->get_width() / 4;
    }
    const uint32_t n = face_size;

    for (int face = 0; face < 6; face++) {
        DynamicArray<std::byte> buffer( static_cast<size_t>( n ) * n * 4 );
        auto* dst = reinterpret_cast<uint8_t*>( buffer.data() );

        for (uint32_t y = 0; y < n; y++) {
            for (uint32_t x = 0; x < n; x++) {
                const float u = ( 2.0f * static_cast<float>( x ) + 1.0f ) / static_cast<float>( n ) - 1.0f;
                const float v = 1.0f - ( 2.0f * static_cast<float>( y ) + 1.0f ) / static_cast<float>( n );

                Vec3 d = face_direction_( face, u, v ).normalize();

                const float eu = std::atan2( d.z, d.x ) / math::DOUBLE_PI + 0.5f;
                const float ev = std::acos( std::clamp( d.y, -1.0f, 1.0f ) ) / math::PI;

                auto sample      = bilinear_sample_( src, ew, eh, eu, ev );
                const size_t idx = ( static_cast<size_t>( y ) * n + x ) * 4;
                dst[idx + 0]     = sample.channels[0];
                dst[idx + 1]     = sample.channels[1];
                dst[idx + 2]     = sample.channels[2];
                dst[idx + 3]     = sample.channels[3];
            }
        }

        faces[face] = Ref<Image>::create( static_cast<int>( n ), static_cast<int>( n ), buffer.data() );
    }
}

ResourceFormatID CubeMap::get_format_id() const
{
    return "cubm";
}

size_t CubeMap::get_size_bytes() const
{
    size_t total = 0;
    for (auto& face : faces) {
        total += face->get_size_bytes();
    }
    return total;
}

Span<const Ref<Image>, 6> CubeMap::get_faces() const
{
    return faces;
}

} // namespace nc
