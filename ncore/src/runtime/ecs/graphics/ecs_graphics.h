#pragma once

#include <ncore/modules/ecs/ecs_module.h>

namespace ncore {

class EcsGraphicsFeature : public EcsFeature {
    NCLASS(EcsGraphicsFeature, EcsFeature)

public:
    void build(EcsWorld& world) override;
};

} // namespace ncore
