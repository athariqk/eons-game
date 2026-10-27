#include "water_mesh.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <ncore/resources/resource.h>
#include <ncore/services/video/renderer/vertex_format.h>

namespace sea {

using namespace nc;

namespace {

constexpr uint64_t kInvalid     = UINT64_MAX;
constexpr uint32_t kMaxVertices = UINT32_MAX;
constexpr uint32_t kMaxLevels   = 24;

struct ClipmapDesc {
    uint32_t cells;
    float base_spacing;
    int32_t half;
    int32_t hole;
    uint32_t levels;
};

// Sanitize parameters + resolve the level count (auto or explicit).
ClipmapDesc sanitize( WaterMeshDesc const& desc_in )
{
    ClipmapDesc d{};

    d.cells = std::max( 4u, desc_in.cells );
    d.cells = ( d.cells + 3u ) & ~3u; // multiple of 4 so the hole edge lands on vertices

    d.base_spacing = std::max( desc_in.base_spacing, 0.01f );
    d.half         = static_cast<int32_t>( d.cells / 2 );
    d.hole         = static_cast<int32_t>( d.cells / 4 );

    uint32_t levels = desc_in.levels;
    if (levels == 0) {
        levels      = 1;
        float reach = static_cast<float>( d.half ) * d.base_spacing * 0.5f;
        while (reach < desc_in.horizon && levels < kMaxLevels) {
            reach *= 2.0f;
            ++levels;
        }
    }
    d.levels = std::clamp( levels, 1u, kMaxLevels );
    return d;
}

struct LevelGeom {
    float sp;
    float half_ext;
    bool has_hole;
    float half_ext_uv;
};

LevelGeom level_geom( ClipmapDesc const& d, uint32_t L )
{
    LevelGeom g{};
    g.sp       = std::ldexp( d.base_spacing, static_cast<int>( L ) - 1 );
    g.half_ext = static_cast<float>( d.half ) * g.sp;
    g.has_hole = L > 0;

    // Outermost level has no coarser parent to morph toward; the shader
    // detects this by a negated half-extent and skips the seam morph.
    g.half_ext_uv = ( L + 1 == d.levels ) ? -g.half_ext : g.half_ext;
    return g;
}

// Fills vertices/indices for the cell rect [ix0,ix1) x [iz0,iz1) of one level.
// Positions stay in level-local space (what the water shader expects) and are
// deduplicated within the rect only, so blocks never share vertices.
void build_level_rect(
    ClipmapDesc const& d, LevelGeom const& g, int32_t ix0, int32_t ix1, int32_t iz0, int32_t iz1,
    DynamicArray<Vertex3D>& vertices, DynamicArray<uint32_t>& indices
)
{
    const uint32_t rw = static_cast<uint32_t>( ix1 - ix0 ) + 1;
    const uint32_t rh = static_cast<uint32_t>( iz1 - iz0 ) + 1;

    DynamicArray<uint64_t> remap( static_cast<size_t>( rw ) * rh );
    std::fill( remap.begin(), remap.end(), kInvalid );

    // Create a vertex the first time a quad references it.
    auto get_vertex = [&]( int32_t ix, int32_t iz ) -> uint32_t {
        uint64_t& slot = remap[static_cast<size_t>( iz - iz0 ) * rw + static_cast<uint32_t>( ix - ix0 )];
        if (slot == kInvalid) {
            slot          = static_cast<uint32_t>( vertices.size() );
            const float x = static_cast<float>( ix ) * g.sp;
            const float z = static_cast<float>( iz ) * g.sp;
            vertices.push_back(
                Vertex3D{
                    x, 0.0f, z,              // position
                    0.0f, 1.0f, 0.0f,        // normal
                    1.0f, 0.0f, 0.0f, -1.0f, // tangent frame (w = -1: bitangent = +Z, matches uv v)
                    x, z,                    // primary UV = local xz
                    g.sp,
                    g.half_ext_uv, // secondary UV = level spacing, ±level half extent (negated on outermost level)
                    0xFFFFFFFF
                }
            );
        }
        return static_cast<uint32_t>( slot );
    };

    for (int32_t iz = iz0; iz < iz1; ++iz) {
        for (int32_t ix = ix0; ix < ix1; ++ix) {
            if (g.has_hole && ix >= -d.hole && ix < d.hole && iz >= -d.hole && iz < d.hole)
                continue;

            const uint32_t v00 = get_vertex( ix, iz );
            const uint32_t v10 = get_vertex( ix + 1, iz );
            const uint32_t v01 = get_vertex( ix, iz + 1 );
            const uint32_t v11 = get_vertex( ix + 1, iz + 1 );

            // Both triangles face +Y
            indices.push_back( v00 );
            indices.push_back( v01 );
            indices.push_back( v11 );

            indices.push_back( v00 );
            indices.push_back( v11 );
            indices.push_back( v10 );
        }
    }
}

} // namespace

nc::Ref<nc::Mesh> create_water_clipmap( WaterMeshDesc const& desc_in )
{
    const ClipmapDesc d = sanitize( desc_in );

    // Chunk block size in cells. Clamp to a divisor of `cells` so blocks tile
    // the level exactly; `chunk_cells == 0` (or unusable) falls back to one
    // chunk per level, which still culls per-level.
    uint32_t block = desc_in.chunk_cells ? std::max( 4u, desc_in.chunk_cells ) : d.cells;
    block          = std::min( block, d.cells );
    while (block > 4 && ( d.cells % block ) != 0)
        block >>= 1;

    const uint32_t blocks = d.cells / block;

    MeshDesc mesh{};
    mesh.vertex_stride = sizeof( Vertex3D );

    DynamicArray<Vertex3D> vertices;

    for (uint32_t L = 0; L < d.levels; ++L) {
        const LevelGeom g = level_geom( d, L );

        for (uint32_t bz = 0; bz < blocks; ++bz) {
            for (uint32_t bx = 0; bx < blocks; ++bx) {
                const int32_t ix0 = -d.half + static_cast<int32_t>( bx * block );
                const int32_t iz0 = -d.half + static_cast<int32_t>( bz * block );
                const int32_t ix1 = ix0 + static_cast<int32_t>( block );
                const int32_t iz1 = iz0 + static_cast<int32_t>( block );

                // Skip blocks entirely inside the ring hole.
                if (g.has_hole && ix0 >= -d.hole && ix1 <= d.hole && iz0 >= -d.hole && iz1 <= d.hole)
                    continue;

                build_level_rect( d, g, ix0, ix1, iz0, iz1, vertices, mesh.indices );
            }
        }
    }

    mesh.vertices = BytesBuffer(
        reinterpret_cast<std::byte const*>( vertices.data() ),
        reinterpret_cast<std::byte const*>( vertices.data() + vertices.size() )
    );

    return Ref<Mesh>::create( mesh );
}

} // namespace sea
