#pragma once

#include <ncore/modules/assets/asset.h>

namespace ncore {

template<std::derived_from<IAssetResource> T>
struct IAssetLoader {
    virtual ~IAssetLoader() = default;

    std::unique_ptr<T> operator()( const std::string_view path )
    {
        auto resource = load( path );
        NC_LOG_TRACE_C( log::IO, "Loaded asset of size {} bytes from path: '{}'", resource->get_size_in_bytes(), path );
        return resource;
    }

    virtual std::unique_ptr<T> load( const std::string_view path ) = 0;
};

} // namespace ncore
