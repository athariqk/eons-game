#pragma once

#include <ncore/kernel/resource.h>
#include <ncore/kernel/structures.h>

namespace ncore {

struct EcsSprite {
    EcsSprite() = default;
    EcsSprite(std::string p_filepath) : filepath(std::move(p_filepath)) {}
    std::string filepath;
    Vec4 rect{};
    RID res;
    float angle = 0;
};

struct EcsCircleDraw {
    float radius = 1.0f;
    Color color{0, 0, 0, 255};
    bool filled = false;
    bool edge   = false;
};

} // namespace ncore
