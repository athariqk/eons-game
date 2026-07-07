#include <ncore/modules/video/resources/mesh.h>

namespace nc {

std::span<std::byte> Mesh::get_vertices()
{
    return vertex_data;
}

std::span<const std::byte> Mesh::get_vertices() const
{
    return vertex_data;
}

std::span<std::byte> Mesh::get_indices()
{
    return index_data;
}

std::span<const std::byte> Mesh::get_indices() const
{
    return index_data;
}

} // namespace nc
