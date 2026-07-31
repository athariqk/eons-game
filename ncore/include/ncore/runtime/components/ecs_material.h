#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>

namespace nc {

class MaterialTemplate;

struct NCAPI EcsMaterialInstance {
    RID source = 0;
    RID instance = 0;
    Array<RID, 8> textures{};

    NSTRUCT(
        EcsMaterialInstance, NC_F( EcsMaterialInstance, source ) NC_F( EcsMaterialInstance, instance )
                                 NC_F( EcsMaterialInstance, textures )
    )
};

} // namespace nc
