#pragma once

#include <ncore/modules/ecs/ecs_module.h>
#include <ncore/modules/ecs/ecs_world.h>

namespace ncore {

class EcsEngineModule : public EcsModule {
    NCLASS(EcsEngineModule, EcsModule)

public:
    void build(EcsWorld &world) override;
};

} // namespace ncore
