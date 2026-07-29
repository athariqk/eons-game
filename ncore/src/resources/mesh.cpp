#include <ncore/resources/mesh.h>

namespace nc {

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

} // namespace nc
