#pragma once

#include <ncore/core/rid.h>
#include <ncore/resources/resource.h>

namespace nc {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"

struct NCAPI HasResourceTag {
    NSTRUCT( HasResourceTag )
};

#pragma clang diagnostic pop

struct NCAPI ResourceLoadedComponent {
    RID resource_id;
    ResourceFormatID format_id;
    NSTRUCT(
        ResourceLoadedComponent, NC_F( ResourceLoadedComponent, resource_id ) NC_F( ResourceLoadedComponent, format_id )
    )
};

} // namespace nc
