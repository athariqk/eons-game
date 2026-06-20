#pragma once

#include <ncore/runtime/ecs/ecs_module.h>
#include <ncore/runtime/ecs_world.h>

namespace ncore {

class EcsEngineModule : public EcsModule {
    NCLASS(EcsEngineModule, EcsModule)

public:
    void build(EcsWorld &world) override;
};

} // namespace ncore
