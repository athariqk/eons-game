#pragma once

#include <ncore/core/rid.h>
#include <ncore/resources/resource.h>

namespace nc {

struct NCAPI HasResourceTag {
    NSTRUCT1( HasResourceTag )
};

struct NCAPI ResourceLoadedComponent {
    RID resource_id;
    ResourceFormatID format_id;
    NSTRUCTV(
        ResourceLoadedComponent, NC_F( ResourceLoadedComponent, resource_id ) NC_F( ResourceLoadedComponent, format_id )
    )
};

} // namespace nc
