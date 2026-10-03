#pragma once

#include <ncore/resources/resource.h>

namespace nc {

/**
 * @brief Res is typed handle to an instance of IResource.
 */
template<std::derived_from<IResource> T>
struct Res {
    RID handle = 0;

    Res() = default;
    Res( const RID& p_handle ) : handle( p_handle ) {}
};

} // namespace nc
