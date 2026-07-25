#include <ncore/resources/mesh.h>

namespace nc {

void Mesh::set_sub_meshes( Vector<SubMesh>&& p_sub_meshes )
{
    sub_meshes = std::move( p_sub_meshes );
}

std::span<const Mesh::SubMesh> Mesh::get_sub_meshes() const
{
    return sub_meshes;
}

std::span<const std::byte> Mesh::get_vertices() const
{
    return vertices;
}

std::span<const std::byte> Mesh::get_indices() const
{
    return indices;
}

const VertexLayout& Mesh::get_vertex_layout() const
{
    return vert_layout;
}

PrimitiveTopology Mesh::get_topology() const
{
    return topology;
}

uint32_t Mesh::get_vertex_stride() const
{
    return vertex_stride;
}

size_t Mesh::vertex_count() const
{
    return vertex_stride ? vertices.size() / vertex_stride : 0;
}

size_t Mesh::index_count() const
{
    return indices.size() / sizeof( uint32_t );
}

} // namespace nc
