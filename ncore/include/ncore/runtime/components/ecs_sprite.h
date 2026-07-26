#pragma once

#include <ncore/core/color.h>
#include <ncore/core/rid.h>
#include <ncore/core/vector.h>

namespace nc {

struct NCAPI EcsSpriteRenderer {
    RID texture;
    Color tint{ 255, 255, 255, 255 };

    NSTRUCT( EcsSpriteRenderer, NC_F( EcsSpriteRenderer, texture ) NC_F( EcsSpriteRenderer, tint ) )
};

struct EcsCircleDraw {
    float radius = 1.0f;
    Color color{ 0, 0, 0, 255 };
    bool filled = false;
    bool edge   = false;
};

} // namespace nc
