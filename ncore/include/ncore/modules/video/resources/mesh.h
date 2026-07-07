#pragma once

#include <ncore/kernel/collection.h>
#include <ncore/modules/resource/resource.h>

namespace nc {

class Mesh : public IResource {
    NCLASS( Mesh, IResource )

public:
    std::span<std::byte> get_vertices();
    std::span<const std::byte> get_vertices() const;

    std::span<std::byte> get_indices();
    std::span<const std::byte> get_indices() const;

private:
    BytesBuffer vertex_data;
    BytesBuffer index_data;
};

} // namespace nc
