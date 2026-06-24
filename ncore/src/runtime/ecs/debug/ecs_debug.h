#pragma once

#include <ncore/modules/ecs/ecs_module.h>
#include <ncore/modules/ecs/ecs_world.h>

namespace ncore {

class EcsDebugFeature : public EcsFeature {
    NCLASS(EcsDebugFeature, EcsFeature)

public:
    void build(EcsWorld &world) override;
};

} // namespace ncore
