#include <ncore/resources/mesh.h>
#include <ncore/services/video/renderer/vertex_format.h>
#include <ncore/utils/assert.h>

namespace nc {

ResourceFormatID Mesh::get_format_id() const
{
    return "mesh";
}

std::span<const std::byte> Mesh::get_vertices() const
{
    return desc.vertices;
}

std::span<const uint16_t> Mesh::get_indices() const
{
    return desc.indices;
}

uint32_t Mesh::get_vertex_stride() const
{
    return desc.vertex_stride;
}

size_t Mesh::vertex_count() const
{
    return desc.vertex_stride ? desc.vertices.size() / desc.vertex_stride : 0;
}

size_t Mesh::index_count() const
{
    return desc.indices.size();
}

constexpr Array<Vertex3D, 24> CUBE_VERTS = {
    //  px,    py,    pz,    nx,    ny,    nz,    tx,   ty,   tz,   tw,   u,    v,    u2,   v2,   color
    Vertex3D{ -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF }
};
constexpr Array<uint16_t, 36> CUBE_INDICES = {
    // Front Face (Z = -1)
    2, 0, 1, 2, 3, 0,
    // Bottom Face (Y = -1)
    6, 4, 5, 6, 7, 4,
    // Right Face (X = +1)
    10, 8, 9, 10, 11, 8,
    // Top Face (Y = +1)
    14, 12, 13, 14, 15, 12,
    // Left Face (X = -1)
    18, 16, 17, 18, 19, 16,
    // Back Face (Z = +1)
    22, 20, 21, 22, 23, 20
};

CubeMesh::CubeMesh() :
    Mesh(
        MeshDesc{
            .vertices = DynamicArray<std::byte>(
                reinterpret_cast<std::byte const*>( CUBE_VERTS.data() ),
                reinterpret_cast<std::byte const*>( CUBE_VERTS.data() + CUBE_VERTS.size() )
            ),
            .indices       = DynamicArray<uint16_t>( CUBE_INDICES.data(), CUBE_INDICES.data() + CUBE_INDICES.size() ),
            .vertex_stride = sizeof( Vertex3D )
        }
    )
{}

PlaneMesh::PlaneMesh( uint32_t x_segments, uint32_t z_segments ) : Mesh( build_mesh_desc_( x_segments, z_segments ) ) {}

MeshDesc PlaneMesh::build_mesh_desc_( uint32_t x_segments, uint32_t z_segments )
{
    NC_ASSERT( x_segments >= 1 && z_segments >= 1, "PlaneMesh segments must be >= 1" );
    NC_ASSERT(
        x_segments <= 255 && z_segments <= 255, "PlaneMesh segments are limited to 255 per side (uint16 indices)"
    );

    const uint32_t vert_count = ( x_segments + 1 ) * ( z_segments + 1 );

    BytesBuffer vertices( static_cast<size_t>( vert_count ) * sizeof( Vertex3D ) );
    auto* verts = reinterpret_cast<Vertex3D*>( vertices.data() );

    for (uint32_t j = 0; j <= z_segments; j++) {
        for (uint32_t i = 0; i <= x_segments; i++) {
            const float x = -1.0f + 2.0f * ( static_cast<float>( i ) / static_cast<float>( x_segments ) );
            const float z = 1.0f - 2.0f * ( static_cast<float>( j ) / static_cast<float>( z_segments ) );
            const float u = static_cast<float>( i ) / static_cast<float>( x_segments );
            const float v = static_cast<float>( j ) / static_cast<float>( z_segments );

            verts[j * ( x_segments + 1 ) + i] =
                Vertex3D{ x, 0.0f, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, u, v, u, v, 0xFFFFFFFF };
        }
    }

    DynamicArray<uint16_t> indices;
    indices.reserve( static_cast<size_t>( x_segments ) * z_segments * 6 );

    for (uint32_t j = 0; j < z_segments; j++) {
        for (uint32_t i = 0; i < x_segments; i++) {
            const uint16_t v00 = static_cast<uint16_t>( j * ( x_segments + 1 ) + i );
            const uint16_t v01 = static_cast<uint16_t>( ( j + 1 ) * ( x_segments + 1 ) + i );
            const uint16_t v10 = static_cast<uint16_t>( j * ( x_segments + 1 ) + i + 1 );
            const uint16_t v11 = static_cast<uint16_t>( ( j + 1 ) * ( x_segments + 1 ) + i + 1 );

            // CCW from +Y (outward).
            indices.push_back( v00 );
            indices.push_back( v11 );
            indices.push_back( v01 );
            indices.push_back( v00 );
            indices.push_back( v10 );
            indices.push_back( v11 );
        }
    }

    return MeshDesc{
        .vertices = std::move( vertices ), .indices = std::move( indices ), .vertex_stride = sizeof( Vertex3D )
    };
}

} // namespace nc
