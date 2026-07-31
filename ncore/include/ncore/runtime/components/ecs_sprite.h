#pragma once

#include <ncore/core/color.h>
#include <ncore/core/rid.h>
#include <ncore/core/vector.h>

namespace nc {

struct NCAPI EcsSpriteInstance {
    RID source;
    RID texture;
    Color tint{ 255, 255, 255, 255 };

    NSTRUCT(
        EcsSpriteInstance,
        NC_F( EcsSpriteInstance, source ) NC_F( EcsSpriteInstance, texture ) NC_F( EcsSpriteInstance, tint )
    )
};

struct EcsCircleDraw {
    float radius = 1.0f;
    Color color{ 0, 0, 0, 255 };
    bool filled = false;
    bool edge   = false;
};

} // namespace nc
