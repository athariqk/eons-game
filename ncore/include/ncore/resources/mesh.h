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

    std::span<const std::byte> get_vertices() const;
    std::span<const uint16_t> get_indices() const;
    size_t vertex_count() const;
    size_t index_count() const;
    uint32_t get_vertex_stride() const;

private:
    MeshDesc desc;
};

} // namespace nc
