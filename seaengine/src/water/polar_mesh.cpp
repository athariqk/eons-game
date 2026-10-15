#include "polar_mesh.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <ncore/resources/resource.h>
#include <ncore/services/video/renderer/vertex_format.h>

namespace sea {

using namespace nc;

Ref<Mesh> create_polar_plane_mesh(
    uint32_t num_segments, uint32_t num_rings, float inner_radius, float outer_radius, float ring_growth,
    float near_radius, float near_spacing
)
{
    num_segments = std::max( 3u, num_segments );
    inner_radius = std::max( 0.01f, inner_radius );
    ring_growth  = std::max( 1.0001f, ring_growth );
    if (outer_radius <= inner_radius)
        outer_radius = inner_radius * 2.0f;

    near_radius = std::clamp( near_radius, inner_radius, outer_radius );

    // Ring radii: a uniform near band (dense enough that world-anchored vertex
    // displacement does not visibly boil while the camera moves), then geometric
    // growth out to outer_radius.
    DynamicArray<float> radii;
    radii.reserve( static_cast<size_t>( num_rings ) + 2 );
    radii.push_back( inner_radius );

    if (near_radius > inner_radius && near_spacing > 1e-4f) {
        float r = inner_radius;
        while (radii.size() < num_rings && r + near_spacing <= near_radius) {
            r += near_spacing;
            radii.push_back( r );
        }
    }

    for (float r = radii.back(); radii.size() < num_rings;) {
        r *= ring_growth;
        if (r >= outer_radius)
            break;
        radii.push_back( r );
    }
    if (radii.back() < outer_radius)
        radii.push_back( outer_radius );

    const uint32_t seg   = num_segments;
    const uint32_t rings = static_cast<uint32_t>( radii.size() );

    DynamicArray<Vertex3D> vertices;
    vertices.reserve( 1 + static_cast<size_t>( rings ) * seg );

    // Center (closed with a fan to the innermost ring).
    vertices.push_back(
        Vertex3D{ 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF }
    );

    for (uint32_t j = 0; j < rings; ++j) {
        const float radius = radii[j];
        for (uint32_t i = 0; i < seg; ++i) {
            const float theta = 2.0f * math::PI * static_cast<float>( i ) / static_cast<float>( seg );
            const float x     = radius * std::cos( theta );
            const float z     = radius * std::sin( theta );
            vertices.push_back(
                Vertex3D{ x, 0.0f, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, x, z, x, z, 0xFFFFFFFF }
            );
        }
    }

    DynamicArray<uint16_t> indices;
    indices.reserve( static_cast<size_t>( seg ) * ( 1 + 2 * ( rings - 1 ) ) * 3 );

    const auto vidx = [seg]( uint32_t ring, uint32_t i ) -> uint16_t {
        return static_cast<uint16_t>( 1 + ring * seg + ( i % seg ) );
    };

    // Center fan.
    for (uint32_t i = 0; i < seg; ++i) {
        indices.push_back( 0 );
        indices.push_back( vidx( 0, i + 1 ) );
        indices.push_back( vidx( 0, i ) );
    }

    // Ring quads.
    for (uint32_t j = 0; j + 1 < rings; ++j) {
        for (uint32_t i = 0; i < seg; ++i) {
            const uint16_t v00 = vidx( j, i );
            const uint16_t v10 = vidx( j, i + 1 );
            const uint16_t v01 = vidx( j + 1, i );
            const uint16_t v11 = vidx( j + 1, i + 1 );
            indices.push_back( v00 );
            indices.push_back( v11 );
            indices.push_back( v01 );
            indices.push_back( v00 );
            indices.push_back( v10 );
            indices.push_back( v11 );
        }
    }

    MeshDesc desc{};
    desc.vertices = BytesBuffer(
        reinterpret_cast<std::byte const*>( vertices.data() ),
        reinterpret_cast<std::byte const*>( vertices.data() + vertices.size() )
    );
    desc.indices       = DynamicArray<uint16_t>( indices.data(), indices.data() + indices.size() );
    desc.vertex_stride = sizeof( Vertex3D );
    return Ref<Mesh>::create( desc );
}

} // namespace sea
