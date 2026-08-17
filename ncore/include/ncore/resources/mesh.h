#pragma once

#include <ncore/core/collection.h>
#include <ncore/resources/resource.h>

namespace nc {

struct NCAPI MeshDesc {
    BytesBuffer vertices;
    DynamicArray<uint16_t> indices;
    uint32_t vertex_stride;
};

/**
 * @brief Mesh is a static geometrical data containing vertex attributes.
 */
class NCAPI Mesh : public IResource {
    NCLASS( Mesh, IResource )

public:
    Mesh( const MeshDesc& p_desc ) : desc( p_desc ) {}

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;

    std::span<const std::byte> get_vertices() const;
    std::span<const uint16_t> get_indices() const;
    size_t vertex_count() const;
    size_t index_count() const;
    uint32_t get_vertex_stride() const;

protected:
    MeshDesc desc;
};

/**
 * @brief A primitve cube mesh.
 */
class NCAPI CubeMesh : public Mesh {
public:
    CubeMesh();
};

/**
 * @brief A primitve plane mesh.
 *
 * The plane lies on the XZ plane (normal +Y) with a unit size of 2x2 (-1..1),
 * subdivided into x_segments * z_segments quads. Limited to 255 segments per
 * side due to 16-bit indices.
 */
class NCAPI PlaneMesh : public Mesh {
public:
    PlaneMesh( uint32_t x_segments = 1, uint32_t z_segments = 1 );

private:
    MeshDesc build_mesh_desc_( uint32_t x_segments, uint32_t z_segments );
};

} // namespace nc
