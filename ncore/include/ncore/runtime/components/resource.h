#pragma once

#include <ncore/core/rid.h>
#include <ncore/resources/resource.h>

namespace nc {

struct NCAPI HasResourceTag {
    NSTRUCT1( HasResourceTag )
};

struct NCAPI ResourceLoadedComponent {
    RID ResourceId;
    ResourceFormatID format_id;
    NSTRUCTV(
        ResourceLoadedComponent, NC_F( ResourceLoadedComponent, ResourceId ),
        NC_F( ResourceLoadedComponent, format_id )
    )
};

} // namespace nc
