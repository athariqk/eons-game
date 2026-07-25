#pragma once

#include <ncore/core/rid.h>
#include <ncore/core/structures.h>

namespace nc {

struct NCAPI EcsSpriteRenderer {
    Vec4 rect{ 0, 0, 1, 1 };
    float angle = 0;

    NSTRUCT( EcsSpriteRenderer, NC_F( EcsSpriteRenderer, rect ) NC_F( EcsSpriteRenderer, angle ) )
};

struct EcsCircleDraw {
    float radius = 1.0f;
    Color color{ 0, 0, 0, 255 };
    bool filled = false;
    bool edge   = false;
};

} // namespace nc
