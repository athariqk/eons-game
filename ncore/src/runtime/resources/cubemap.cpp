// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <ncore/runtime/resources/cubemap.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/servicedb.h>
#include <ncore/services/video/render_service.h>

#include "service_helpers.h"

namespace nc {

struct RGBA8 {
    uint8_t channels[4];
};

static RGBA8 bilinear_sample( const uint8_t* pixels, int w, int h, float eu, float ev )
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

static Vec3f get_face_direction( int face, float u, float v )
{
    switch (face) {
        case 0:
            return Vec3f( 1.0f, v, -u );  // +X
        case 1:
            return Vec3f( -1.0f, v, u );  // -X
        case 2:
            return Vec3f( u, 1.0f, -v );  // +Y
        case 3:
            return Vec3f( u, -1.0f, v );  // -Y
        case 4:
            return Vec3f( u, v, 1.0f );   // +Z
        default:
            return Vec3f( -u, v, -1.0f ); // -Z
    }
}

CubeMap::CubeMap( const Ref<Image>& equirect, uint32_t p_face_size )
{
    const int ew   = static_cast<int>( equirect->get_width() );
    const int eh   = static_cast<int>( equirect->get_height() );
    const auto src = reinterpret_cast<const uint8_t*>( equirect->get_pixels().data() );

    if (p_face_size == 0) {
        p_face_size = equirect->get_width() / 4;
    }
    const uint32_t n        = p_face_size;
    const size_t face_bytes = static_cast<size_t>( n ) * n * 4;

    this->face_size = static_cast<int>( n );
    pixels.resize( face_bytes * 6 );

    auto* dst_base = reinterpret_cast<uint8_t*>( pixels.data() );

    for (int face = 0; face < 6; face++) {
        uint8_t* dst = dst_base + static_cast<size_t>( face ) * face_bytes;

        for (uint32_t y = 0; y < n; y++) {
            for (uint32_t x = 0; x < n; x++) {
                const float u = ( 2.0f * static_cast<float>( x ) + 1.0f ) / static_cast<float>( n ) - 1.0f;
                const float v = 1.0f - ( 2.0f * static_cast<float>( y ) + 1.0f ) / static_cast<float>( n );

                Vec3f d = get_face_direction( face, u, v ).normalize();

                const float eu = std::atan2( d.z, d.x ) / math::DOUBLE_PI + 0.5f;
                const float ev = std::acos( std::clamp( d.y, -1.0f, 1.0f ) ) / math::PI;

                auto sample      = bilinear_sample( src, ew, eh, eu, ev );
                const size_t idx = ( static_cast<size_t>( y ) * n + x ) * 4;
                dst[idx + 0]     = sample.channels[0];
                dst[idx + 1]     = sample.channels[1];
                dst[idx + 2]     = sample.channels[2];
                dst[idx + 3]     = sample.channels[3];
            }
        }
    }
}

CubeMap::~CubeMap()
{
    release();
}

ResourceFormatID CubeMap::get_format_id() const
{
    return "ncbm";
}

size_t CubeMap::get_size_bytes() const
{
    return pixels.size();
}

RID CubeMap::prepare( Scene& p_scene )
{
    if (is_prepared_alive( renderer, prepared_rid, service_revision ))
        return prepared_rid;

    RenderService* renderer_now = p_scene.get_app_ctx()->services.resolve<RenderService>();
    if (!renderer_now || face_size <= 0 || pixels.empty())
        return prepared_rid;

    release();

    TextureCubeDesc desc{};
    desc.face_size  = static_cast<uint32_t>( face_size );
    desc.format     = gfx::TextureFormat::RGBA8_UNORM_SRGB;
    desc.faces      = Span<const std::byte>( pixels.data(), pixels.size() );
    desc.debug_name = StringView( filepath );

    RID rid = renderer_now->texture_cube_create( desc );
    if (!rid.is_valid())
        return prepared_rid;

    renderer = renderer_now;
    set_prepared( rid, renderer_now->get_device_revision() );
    return prepared_rid;
}

void CubeMap::release()
{
    if (is_prepared_alive( renderer, prepared_rid, service_revision ))
        renderer->destroy_rid( prepared_rid );
    prepared_rid = RID::INVALID;
    renderer     = nullptr;
}

uint32_t CubeMap::get_face_size() const
{
    return static_cast<uint32_t>( face_size );
}

Span<const std::byte> CubeMap::get_face_data( uint32_t face ) const
{
    const size_t face_bytes = static_cast<size_t>( face_size ) * static_cast<size_t>( face_size ) * 4;
    if (face >= 6 || face_bytes == 0 || pixels.size() < face_bytes * 6)
        return {};
    return Span<const std::byte>( pixels ).subspan( static_cast<size_t>( face ) * face_bytes, face_bytes );
}

} // namespace nc
