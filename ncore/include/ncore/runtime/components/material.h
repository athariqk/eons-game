#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>

namespace nc {

class MaterialTemplate;

struct NCAPI MaterialComponent {
    RID source   = 0;
    RID instance = 0;
    Array<RID, 8> textures{};

    NSTRUCT(
        MaterialComponent,
        NC_F( MaterialComponent, source ) NC_F( MaterialComponent, instance ) NC_F( MaterialComponent, textures )
    )
};

} // namespace nc
