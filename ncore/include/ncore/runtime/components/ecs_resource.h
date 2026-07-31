#pragma once

#include <ncore/core/rid.h>

namespace nc {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"

struct NCAPI EcsHasResource {
    NSTRUCT( EcsHasResource )
};

#pragma clang diagnostic pop

struct NCAPI EcsResourceLoaded {
    RID resource_id;
    ResourceFormatID format_id;
    NSTRUCT( EcsResourceLoaded, NC_F( EcsResourceLoaded, resource_id ) NC_F( EcsResourceLoaded, format_id ) )
};

} // namespace nc
