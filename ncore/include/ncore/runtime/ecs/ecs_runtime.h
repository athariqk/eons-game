#pragma once

#include <ncore/modules/ecs/ecs_module.h>
#include <ncore/modules/ecs/ecs_world.h>

namespace ncore {

class EcsRuntime : public EcsModule {
    NCLASS(EcsRuntime, EcsModule)

public:
    EcsRuntime() {}
    void build(EcsWorld &world) override;
};

} // namespace ncore
