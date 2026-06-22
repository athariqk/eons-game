#pragma once

#include <ncore/modules/ecs/ecs_module.h>
#include <ncore/modules/ecs/ecs_world.h>

namespace ncore {

class EcsDebug : public EcsModule {
    NCLASS(EcsDebug, EcsModule)

public:
    void build(EcsWorld &world) override;
};

} // namespace ncore
