#pragma once

#include <ncore/modules/ecs/ecs_module.h>

namespace ncore {

class EcsPhysics : public EcsModule {
    NCLASS(EcsPhysics, EcsModule)

public:
    void build(EcsWorld &world) override {}
};

} // namespace ncore
