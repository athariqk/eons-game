#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>

namespace nc {

class MaterialTemplate;

struct NCAPI EcsMaterialInstance {
    Ref<MaterialTemplate> template_resource;
    RID material = 0;
    Array<RID, 8> textures{};

    NSTRUCT(
        EcsMaterialInstance, NC_F( EcsMaterialInstance, template_resource ) NC_F( EcsMaterialInstance, material )
                                 NC_F( EcsMaterialInstance, textures )
    )
};

} // namespace nc
