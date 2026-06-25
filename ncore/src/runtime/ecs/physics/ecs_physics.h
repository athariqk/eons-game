#pragma once

#include <ncore/modules/ecs/ecs_module.h>

namespace ncore {

class EcsPhysicsFeature : public EcsFeature {
    NCLASS(EcsPhysicsFeature, EcsFeature)

public:
    void build(EcsWorld& world) override {}
};

} // namespace ncore
